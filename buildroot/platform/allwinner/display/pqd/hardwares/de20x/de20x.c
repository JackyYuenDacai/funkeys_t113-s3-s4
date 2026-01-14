/*
 * Copyright (C) 2022  Allwinnertech
 * Author: yajianz <yajianz@allwinnertech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "fcm/fcm.h"
#include "hardwares/driver_api.h"
#include "pqal.h"
#include "sockets/protocol.h"
#include "pqdata/config.h"

static int get_dispdev_fd(void) {
    static int devfd = -1;
    if (devfd < 0) {
        devfd = open("/dev/disp", O_RDWR);
    }
    return devfd;
}

// ------------------------------------------------------------------------------------------//
//                                 General Register RW                                       //
// ------------------------------------------------------------------------------------------//

static void register_packet_to_ioctl_data(
        const struct register_data* data, int count, struct register_info* out)
{
    for (int i = 0; i < count; i++) {
        out->offset = data->offset;
        out->value  = data->value;
        out++;
        data++;
    }
}

static void register_packet_from_ioctl_data(
        struct register_data* out, int count, const struct register_info* info)
{
    for (int i = 0; i < count; i++) {
        out->offset = info->offset;
        out->value  = info->value;
        out++;
        info++;
    }
}

int de20x_set_registers(const struct register_data* data, int count)
{
    struct register_info* infos = malloc(sizeof(struct register_info) * count);
    register_packet_to_ioctl_data(data, count, infos);

    const struct hardware_data* hwdat = pq_hardware_data();
    unsigned long arg[4] = {0};

    arg[0] = (unsigned long)PQ_SET_REG;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)infos;
    arg[3] = (unsigned long)count;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC PQ_SET_REG failed: %d", ret);
        free(infos);
        return -1;
    }

    free(infos);
    return 0;
}

int de20x_get_registers(struct register_data* data, int count)
{
    struct register_info* infos = malloc(sizeof(struct register_info) * count);

    register_packet_to_ioctl_data(data, count, infos);
    const struct hardware_data* hwdat = pq_hardware_data();
    unsigned long arg[4] = {0};

    arg[0] = (unsigned long)PQ_GET_REG;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)infos;
    arg[3] = (unsigned long)count;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC PQ_GET_REG failed: %d", ret);
        free(infos);
        return -1;
    }

    register_packet_from_ioctl_data(data, count, infos);
    free(infos);
    return 0;
}

// ------------------------------------------------------------------------------------------//
//                                     Gamma lut                                             //
// ------------------------------------------------------------------------------------------//

static int packet_to_gamma_lut(const char* data, int size, struct hw_gamma_lut* out)
{
    if (size < 256 * 3) {
        dloge("packet size error, convert to hardware gamma lut failed!");
        return -1;
    }

    int i = 0;
    const unsigned char* p = (const unsigned char*)data;
    while (i < 256) {
        out->x[i] = (((uint32_t)p[i]) << 16) | (((uint32_t)p[i + 256]) << 8) | (p[i + 512]);
        i++;
    }
    return 0;
}

static int gamma_lut_to_packet(const struct hw_gamma_lut* lut, char* data)
{
    int i = 0;
    while (i < 256) {
        data[i]       = (char)(lut->x[i] >> 16) & 0xff;
        data[i + 256] = (char)(lut->x[i] >>  8) & 0xff;
        data[i + 512] = (char)(lut->x[i]      ) & 0xff;
        i++;
    }
    return 0;
}

int de20x_set_gamma_lut(const char *data, int size)
{
    const struct payload_head *head = (struct payload_head *)data;
    size -= sizeof(struct payload_head);
    struct hw_gamma_lut lut = {0};
    packet_to_gamma_lut(head->data, size, &lut);

    unsigned long arg[4] = {0};
    int ret = ioctl(get_dispdev_fd(), DISP_LCD_GAMMA_CORRECTION_ENABLE, arg);
    if (ret) {
        dloge("DISP_LCD_GAMMA_CORRECTION_ENABLE failed: %d", ret);
        return -1;
    }

    const struct hardware_data *hwdat = pq_hardware_data();

    arg[0] = (unsigned long)hwdat->port;
    arg[1] = (unsigned long)&lut;
    arg[2] = (unsigned long)sizeof(lut);

    ret = ioctl(get_dispdev_fd(), DISP_LCD_SET_GAMMA_TABLE, (unsigned long)arg);
    if (ret) {
        dloge("DISP_LCD_GAMMA_TABLE failed: %d", ret);
        return -1;
    }
    return 0;
}

int de20x_get_gamma_lut(char *data, int size)
{
    const struct hardware_data *hwdat = pq_hardware_data();
    struct hw_gamma_lut lut = {0};
    unsigned long arg[4] = {0};

    UNUSED(size);
    arg[0] = (unsigned long)hwdat->port;
    arg[1] = (unsigned long)&lut;
    arg[2] = (unsigned long)sizeof(lut);

    int ret = ioctl(get_dispdev_fd(), DISP_LCD_GET_GAMMA_TABLE, (unsigned long)arg);
    if (ret) {
        dloge("DISP_LCD_GET_GAMMA_TABLE failed: %d", ret);
        return -1;
    }
    struct payload_head *head = (struct payload_head *)data;
    gamma_lut_to_packet(&lut, head->data);

    return 0;
}
// ------------------------------------------------------------------------------------------//
//                                        PEAK                                               //
// ------------------------------------------------------------------------------------------//
#define PEAK_OFFSET	0xA6000	/* PEAKING offset based on RTMX */
#define PEAK_REG_NUM	6

struct peak_sharp {
    int id;
    char enable;
    char gain;
    char neg_gain;
    char hp_ratio;
    char bp0_ratio;
    char bp1_ratio;
    char dif_up;
    char corth;
    char beta;
}__attribute__ ((packed));

static void get_peak_register_offset(struct register_data *data)
{
    data[0].offset = PEAK_OFFSET;                /* enable */
    data[1].offset = PEAK_OFFSET + 0x10;         /* hp_ratio; bp0_ratio; bp1_ratio; */
    data[2].offset = PEAK_OFFSET + 0x20;         /* gain */
    data[3].offset = PEAK_OFFSET + 0x24;         /* dif_up; beta */
    data[4].offset = PEAK_OFFSET + 0x28;         /* neg_gain */
    data[5].offset = PEAK_OFFSET + 0x2c;         /* corth */
}

int de20x_set_peak(const char* data, int size)
{
    int ret;
    UNUSED(size);
    struct peak_sharp *peak = (struct peak_sharp *)data;
    struct register_data* rdata = malloc(sizeof(struct register_data) * PEAK_REG_NUM);
    get_peak_register_offset(rdata);
    ret = de20x_get_registers(rdata, PEAK_REG_NUM);
    if (ret) {
        free(rdata);
        return ret;
    }

    rdata[0].value &= 0xfffffffe;
    rdata[0].value |= !!peak->enable;
    rdata[1].value &= 0xff000000;
    rdata[1].value |= (peak->hp_ratio << 16) + (peak->bp0_ratio << 8 ) + peak->bp1_ratio;
    rdata[2].value &= 0xffffff00;
    rdata[2].value |= peak->gain;
    rdata[3].value &= 0xff000000;
    rdata[3].value |= (peak->dif_up << 16) + peak->beta;
    rdata[4].value &= 0xffffffa0;
    rdata[4].value |= peak->neg_gain;
    rdata[5].value &= 0xffffff00;
    rdata[5].value |= peak->corth;

    ret = de20x_set_registers(rdata, PEAK_REG_NUM);
    free(rdata);
    return ret;
}

int de20x_get_peak(char* data, int size)
{
    int ret;
    UNUSED(size);
    struct peak_sharp *peak = (struct peak_sharp *)data;
    struct register_data* rdata = malloc(sizeof(struct register_data) * PEAK_REG_NUM);
    get_peak_register_offset(rdata);
    ret = de20x_get_registers(rdata, PEAK_REG_NUM);

    peak->enable = (rdata[0].value) & 0x1;
    peak->hp_ratio = (rdata[1].value >> 16) & 0x3f;
    peak->bp0_ratio = (rdata[1].value >> 8) & 0x3f;
    peak->bp1_ratio = (rdata[1].value) & 0x3f;
    peak->gain = rdata[2].value & 0xff;
    peak->dif_up = (rdata[3].value >> 16) & 0xff;
    peak->beta = rdata[3].value & 0xff;
    peak->neg_gain = rdata[4].value & 0x3f;
    peak->corth = rdata[5].value & 0xff;

    free(rdata);
    return ret;
}

// ------------------------------------------------------------------------------------------//
//                                         LTI                                               //
// ------------------------------------------------------------------------------------------//
#define LTI_OFFSET	0xA4000	/* LTI offset based on RTMX */
#define LTI_REG_NUM	11

struct lti_sharp {
    int id;
    char enable;
    char cstm_coeff0;
    char cstm_coeff1;
    char cstm_coeff2;
    char cstm_coeff3;
    char cstm_coeff4;
    char fir_gain;
    short cor_th;
    char diff_slope;
    char diff_offset;
    char edge_gain;
    char sel;
    char win_expansion;
    char nolinear_limmit_en;
    char peak_limmit;
    char core_x;
    char clip_y;
    char edge_level_th;
}__attribute__ ((packed));

static void get_lti_register_offset(struct register_data *data)
{
	data[0].offset = LTI_OFFSET;          /* bit 0: enable, bit 8: sel, bit 16: nolinear_limmit_en */
	data[1].offset = LTI_OFFSET + 0x10;   /* bit 0~7: cstm_coeff0, bit 16~23: cstm_coeff1 */
	data[2].offset = LTI_OFFSET + 0x14;   /* bit 0~7: cstm_coeff2, bit 16~23: cstm_coeff3 */
	data[3].offset = LTI_OFFSET + 0x18;   /* bit 0~7: cstm_coeff4 */
	data[4].offset = LTI_OFFSET + 0x1c;   /* bit 0~3: fir_gain */
	data[5].offset = LTI_OFFSET + 0x20;   /* bit 0~9: cor_th */
	data[6].offset = LTI_OFFSET + 0x24;   /* bit 0~7: diff_offset, bit 16~20:diff_slope */
	data[7].offset = LTI_OFFSET + 0x28;   /* bit 0~4: edge_gain */
	data[8].offset = LTI_OFFSET + 0x2c;   /* bit 0~7: core_x, bit 16~23: clip_y, bit 28~30: peak_limmit */
	data[9].offset = LTI_OFFSET + 0x30;   /* bit 0~7: win_expansion */
	data[10].offset = LTI_OFFSET + 0x34 ; /* bit 0~7: edge_level_th */
 }

int de20x_set_lti(const char* data, int size)
{
    int ret;
    UNUSED(size);
    struct lti_sharp *lti = (struct lti_sharp *)data;
    struct register_data* rdata = malloc(sizeof(struct register_data) * LTI_REG_NUM);
    get_lti_register_offset(rdata);
    ret = de20x_get_registers(rdata, LTI_REG_NUM);
    if (ret) {
        free(rdata);
        return ret;
    }

    rdata[0].value &= 0xfffefefe;
    rdata[0].value |= (!!lti->enable) | (((unsigned int)(!!lti->sel)) << 8)
                        | ((unsigned int)!!lti->nolinear_limmit_en) << 16;
    rdata[1].value &= 0xff00ff00;
    rdata[1].value |= (((int)lti->cstm_coeff1) << 16) + lti->cstm_coeff0;
    rdata[2].value &= 0xff00ff00;
    rdata[2].value |= (((int)lti->cstm_coeff3) << 16) + lti->cstm_coeff2;
    rdata[3].value &= 0xffffff00;
    rdata[3].value |= lti->cstm_coeff4;
    rdata[4].value &= 0xfffffff0;
    rdata[4].value |= lti->fir_gain;
    rdata[5].value &= 0xfffffc00;
    rdata[5].value |= lti->cor_th;
    rdata[6].value &= 0xff00ff00;
    rdata[6].value |= ((int)lti->diff_slope) << 16 | lti->diff_offset;
    rdata[7].value &= 0xfffffff0;
    rdata[7].value |= lti->edge_gain;
    rdata[8].value &= 0x8f00ff00;
    rdata[8].value |= lti->core_x | (((unsigned int)lti->clip_y) << 16)
                        | (((unsigned int)lti->peak_limmit) << 28);
    rdata[9].value &= 0xffffff00;
    rdata[9].value |= lti->win_expansion;
    rdata[10].value &= 0xffffff00;
    rdata[10].value |= lti->edge_level_th;

    ret = de20x_set_registers(rdata, LTI_REG_NUM);
    free(rdata);
    return ret;
}

int de20x_get_lti(char* data, int size)
{
    int ret;
    UNUSED(size);
    struct lti_sharp *lti = (struct lti_sharp *)data;
    struct register_data* rdata = malloc(sizeof(struct register_data) * LTI_REG_NUM);
    get_lti_register_offset(rdata);
    ret = de20x_get_registers(rdata, LTI_REG_NUM);
    if (ret) {
        free(rdata);
        return ret;
    }

    lti->enable = (rdata[0].value) & 0x1;
    lti->sel = !!(rdata[0].value & 0x100);
    lti->nolinear_limmit_en = !!(rdata[0].value & 0x10000);
    lti->cstm_coeff0 = rdata[1].value & 0xff;
    lti->cstm_coeff1 = (rdata[1].value >> 16) & 0xff;
    lti->cstm_coeff2 = rdata[2].value & 0xff;
    lti->cstm_coeff3 = (rdata[2].value >> 16) & 0xff;
    lti->cstm_coeff4 = rdata[3].value & 0xff;
    lti->fir_gain = rdata[4].value & 0xf;
    lti->cor_th = rdata[5].value & 0x3ff;
    lti->diff_offset = rdata[6].value & 0xff;
    lti->diff_slope = (rdata[6].value >> 16) & 0x1f;
    lti->edge_gain = rdata[7].value & 0x1f;
    lti->core_x = rdata[8].value & 0xff;
    lti->clip_y = (rdata[8].value >> 16) & 0xff;
    lti->peak_limmit = (rdata[8].value >> 28) & 0x07;
    lti->win_expansion = rdata[9].value & 0xff;
    lti->edge_level_th = rdata[10].value & 0xff;

    free(rdata);
    return ret;
}

// ------------------------------------------------------------------------------------------//
//                                     Color Enhance                                         //
// ------------------------------------------------------------------------------------------//

/* packet from pqtool */
struct enhance_packet {
    int id;
    char contrast;
    char brightness;
    char saturation;
    char hue;
} __attribute__((packed));

int de20x_set_enhance(const char *data, int size)
{
    const struct enhance_packet *packet = (const struct enhance_packet *)data;
    struct enhance_data enhance;

    UNUSED(size);
    enhance.contrast = packet->contrast;
    enhance.brightness = packet->brightness;
    enhance.saturation = packet->saturation;
    enhance.hue = packet->hue;
    enhance.cmd = 0;
    enhance.read = 0;

    const struct hardware_data *hwdat = pq_hardware_data();
    unsigned long arg[4] = {0};
    arg[0] = (unsigned long)PQ_COLOR_MATRIX;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)&enhance;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC PQ_COLOR_MATRIX failed: %d", ret);
        return -1;
    }
    return 0;
}

int de20x_get_enhance(char *data, int size)
{
    const struct hardware_data *hwdat = pq_hardware_data();
    struct enhance_data enhance;

    UNUSED(size);
    memset(&enhance, 0, sizeof(enhance));
    enhance.cmd = 0;
    enhance.read = 1;

    unsigned long arg[4] = {0};
    arg[0] = (unsigned long)PQ_COLOR_MATRIX;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)&enhance;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC PQ_COLOR_MATRIX failed: %d", ret);
        return -1;
    }

    struct enhance_packet *packet = (struct enhance_packet *)data;
    packet->contrast = enhance.contrast;
    packet->brightness = enhance.brightness;
    packet->saturation = enhance.saturation;
    packet->hue = enhance.hue;

    return 0;
}

// ------------------------------------------------------------------------------------------//
//                                     Color Matrix                                          //
// ------------------------------------------------------------------------------------------//

/*
 * matrix from pqtool to pqd
 */
struct matrix_packet {
    int id;
    char space;
    char data[0]; /* float * 12 */
} __attribute__((packed));

struct float_matrix {
    float c00;
    float c01;
    float c02;
    float c10;
    float c11;
    float c12;
    float c20;
    float c21;
    float c22;
    float offset0;
    float offset1;
    float offset2;
};

static int packet_to_matrix(const struct float_matrix* in, struct matrix4x4* matrix)
{
    matrix->x00 = ceil((double)(in->c00) * 4096);
    matrix->x01 = ceil((double)(in->c01) * 4096);
    matrix->x02 = ceil((double)(in->c02) * 4096);
    matrix->x03 = ceil((double)(in->offset0) * 4096);

    matrix->x10 = ceil((double)(in->c10) * 4096);
    matrix->x11 = ceil((double)(in->c11) * 4096);
    matrix->x12 = ceil((double)(in->c12) * 4096);
    matrix->x13 = ceil((double)(in->offset1) * 4096);

    matrix->x20 = ceil((double)(in->c20) * 4096);
    matrix->x21 = ceil((double)(in->c21) * 4096);
    matrix->x22 = ceil((double)(in->c22) * 4096);
    matrix->x23 = ceil((double)(in->offset2) * 4096);

    matrix->x30 = 0;
    matrix->x31 = 0;
    matrix->x32 = 0;
    matrix->x33 = 0x00001000;

    return 0;
}

static int matrix_to_packet(const struct matrix4x4* matrix, struct float_matrix* out)
{
    out->c00 = matrix->x00 / 4096.0;
    out->c01 = matrix->x01 / 4096.0;
    out->c02 = matrix->x02 / 4096.0;
    out->c10 = matrix->x10 / 4096.0;
    out->c11 = matrix->x11 / 4096.0;
    out->c12 = matrix->x12 / 4096.0;
    out->c20 = matrix->x20 / 4096.0;
    out->c21 = matrix->x21 / 4096.0;
    out->c22 = matrix->x22 / 4096.0;
    out->offset0 = matrix->x03 / 4096.0;
    out->offset1 = matrix->x13 / 4096.0;
    out->offset2 = matrix->x23 / 4096.0;

    return 0;
}

int de20x_set_color_matrix(const char *data, int size)
{
    const struct matrix_packet *mp = (struct matrix_packet *)data;
    size -= sizeof(struct matrix_packet);
    struct float_matrix fmatrix = {0};
    struct matrix_user mu = {0};

    if (size < sizeof(struct float_matrix)) {
        dloge("matrix packet size=%d, sizeof(struct float_matrix)=%zu",
                size, sizeof(struct float_matrix));
        return -1;
    }
    dlogi("matrix packet size=%d, sizeof(struct float_matrix)=%zu",
            size, sizeof(struct float_matrix));

    memcpy(&fmatrix, mp->data, sizeof(fmatrix));
    packet_to_matrix(&fmatrix, &mu.matrix);
    mu.cmd  = mp->id == COLOR_MATRIX_IN ? 1 : 2;
    mu.read = 0;
    mu.choice = mp->space;

    const struct hardware_data *hwdat = pq_hardware_data();

    unsigned long arg[4] = {0};
    arg[0] = (unsigned long)PQ_COLOR_MATRIX;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)&mu;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC failed: %d", ret);
        return -1;
    }

    return 0;
}

int de20x_get_color_matrix(char *data, int size)
{
    struct matrix_packet *mp = (struct matrix_packet *)data;
    struct matrix_user mu = {0};
    struct float_matrix fmatrix = {0};
    const struct hardware_data *hwdat = pq_hardware_data();

    UNUSED(size);
    mu.cmd = mp->id == COLOR_MATRIX_IN ? 1 : 2;
    mu.read = 1;
    mu.choice = mp->space;

    unsigned long arg[4] = {0};
    arg[0] = (unsigned long)PQ_COLOR_MATRIX;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)&mu;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC failed: %d", ret);
        return -1;
    }

    matrix_to_packet(&mu.matrix, &fmatrix);
    memcpy(mp->data, &fmatrix, sizeof(fmatrix));
    return 0;
}

// ------------------------------------------------------------------------------------------//
//                                          FCM                                              //
// ------------------------------------------------------------------------------------------//

int de20x_get_fcm_packet_index(const char *data, int size)
{
    struct fcm_ctrl_para para;

    if (size < sizeof(para)) {
        dloge("packet size error, size=%d sizeof(para)=%zu", size, sizeof(para));
        return -1;
    }
    memcpy(&para, data, sizeof(para));
    return para.LUT_ID;
}

/*
 * cmd: 0 -- update fcm table and enable this table
 *      1 -- update fcm table but DO NOT enable it
 *      2 -- read fcm table from hardware
 */
static int de20x_set_fcm_internal(const char* data, int size, int cmd)
{
    if (size < sizeof(struct fcm_ctrl_para)) {
        dloge("packet size error, size=%d sizeof(para)=%zu", size, sizeof(struct fcm_ctrl_para));
        return -1;
    }
    dloge("fcm packet size=%d sizeof(para)=%zu", size, sizeof(struct fcm_ctrl_para));

    struct fcm_info* info = malloc(sizeof(*info));
    struct fcm_ctrl_para* para = malloc(sizeof(*para));
    memcpy(para, data, sizeof(*para));
    if (debug_is_verbose_mode()) dump_fcm_ctrl_data("pqtool input", para);

    info->cmd = cmd;
    fcm_convert_pqtool_input(para, &info->fcm_data, debug_is_verbose_mode());

    const struct hardware_data* hwdat = pq_hardware_data();
    unsigned long arg[4] = {0};

    arg[0] = (unsigned long)PQ_FCM;
    arg[1] = (unsigned long)hwdat->port;
    arg[2] = (unsigned long)info;

    int ret = ioctl(get_dispdev_fd(), DISP_PQ_PROC, (unsigned long)arg);
    if (ret) {
        dloge("DISP_PQ_PROC PQ_FCM failed: %d port=%d lut_id=%d", ret, hwdat->port, info->fcm_data.lut_id);
        return -1;
    }

    free(info);
    free(para);

    return 0;
}

int de20x_set_fcm(const char* data, int size, int cmd)
{
    UNUSED(cmd);
    return de20x_set_fcm_internal(data, size, 0);
}

int de20x_init_fcm_hardware_table(const char* data, int size)
{
    return de20x_set_fcm_internal(data, size, 1);
}

int de20x_get_fcm(char *data, int size)
{
    UNUSED(size);
    UNUSED(data);
    /*
     * we can not convert hardware register data to pqtool fcm packet data,
     * so find data from the store pqdata.
     */
    return 0;
}

int dumpCdcData(struct cdc_dat *dat) {
    ssize_t n = 0;
    char tmp[64] = {0};
    int LEN[8] = {729, 648, 648, 576, 648, 576, 576, 512};
    int offset = 0;
    int fd = open(CDCDATA_FILE_NAME, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    if (fd < 0) {
            dloge("open %s err,%s(%d) !", CDCDATA_FILE_NAME, strerror(errno), errno);
            return -1;
    }

    sprintf(tmp, "cmd=0x%08x,\n", dat->cmd);
    n += write(fd, tmp, strlen(tmp));
    for(int j = 0; j < 8; j++) {
            sprintf(tmp, "CDC_LUT%d = {\n", j);
            n += write(fd, tmp, strlen(tmp));
            for(int i = 0; i < LEN[j]; i++) {
                    sprintf(tmp, "0x%08x,\n", *(dat->CDC_LUT0 + offset));
                    n += write(fd, tmp, strlen(tmp));
                    offset++;
            }
            sprintf(tmp, "},\n");
            n += write(fd, tmp, strlen(tmp));
    }

    if (n < sizeof(struct cdc_dat)) {
            dloge("fwrite err, only %zu in\n", n);
            return -1;
    }
    fsync(fd);
    close(fd);
    return 0;
}

int de30x_set_cdc(const char* data, int size, int cmd)
{
    struct cdc_dat dat;
    unsigned long arg[4] = {0};
    const struct hardware_data *hwdat = pq_hardware_data();
    int ret = -1;
    memcpy(&dat, data, sizeof(struct cdc_dat));

    UNUSED(cmd);
    if (size != sizeof(struct cdc_dat))
        dlogw("maybe it's not a cdc package!");

    arg[0] = (unsigned long)hwdat->port;
    arg[1] = (unsigned long)&dat;
    arg[2] = (unsigned long)sizeof(struct cdc_dat);

    dumpCdcData(&dat);
    ret = ioctl(get_dispdev_fd(), DISP_CDC_SET_DAT, (unsigned long)arg);
    if (ret) {
        dloge("DISP_CDC_SET_DAT failed: %d", ret);
        return -1;
    }

    return 0;
}

int de30x_get_cdc(char *data, int size)
{
    const struct hardware_data *hwdat = pq_hardware_data();
    struct cdc_dat dat;
    //struct payload_head *head = (struct payload_head *)data;
    unsigned long arg[4] = {0};
    int ret = -1;

    UNUSED(size);

    arg[0] = (unsigned long)hwdat->port;
    arg[1] = (unsigned long)&dat;
    arg[2] = (unsigned long)sizeof(struct cdc_dat);

    ret = ioctl(get_dispdev_fd(), DISP_CDC_GET_DAT, (unsigned long)arg);
    if (ret) {
        dloge("DISP_CDC_GET_DAT failed: %d", ret);
        return -1;
    }
    //dumpCdcData(&dat);
    memcpy(data, &dat, sizeof(struct cdc_dat));

    return 0;
}

int de30x_get_cdc_packet_index(const char *data, int size)
{
    struct cdc_dat dat;

    if (size < sizeof(struct cdc_dat)) {
        dloge("packet size error, size=%d sizeof(para)=%zu", size, sizeof(struct cdc_dat));
        return -1;
    }
    memcpy(&dat, data, sizeof(struct cdc_dat));
    return PQ_CDC;
}

int de30x_set_fce_be(const char* data, int size, int cmd)
{
    unsigned long arg[4] = {0};
    int ret = -1;
    struct black_extension *black_exten = (struct black_extension *)data;
    const struct hardware_data *hwdat = pq_hardware_data();

    UNUSED(size);
    UNUSED(cmd);
    arg[0] = (unsigned long)hwdat->port;
    arg[1] = (unsigned long)black_exten;
    ret = ioctl(get_dispdev_fd(), DISP_LCD_SET_BLACK_EXTENSION, (unsigned long)arg);
    if (ret) {
        dloge("err: %d set black_exten failed: %d  %d", get_dispdev_fd(), ret, __LINE__);
        return -1;
    }

    return 0;
}

int de30x_get_fce_be(char *data, int size)
{
    unsigned long arg[4] = {0};
    int ret = -1;
    struct black_extension *black_exten = (struct black_extension *)data;
    const struct hardware_data *hwdat = pq_hardware_data();

    UNUSED(size);
    arg[0] = (unsigned long)hwdat->port;
    arg[1] = (unsigned long)black_exten;
    ret = ioctl(get_dispdev_fd(), DISP_LCD_GET_BLACK_EXTENSION, (unsigned long)arg);
    if (ret) {
        dloge("err: %d set black_exten failed: %d  %d", get_dispdev_fd(), ret, __LINE__);
        return -1;
    }
    return 0;
}
