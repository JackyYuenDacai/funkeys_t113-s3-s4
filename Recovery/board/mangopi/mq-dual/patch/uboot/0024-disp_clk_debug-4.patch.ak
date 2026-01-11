From cede2874fc7cf95bbfa0e6fce79a1014fb16f4bf Mon Sep 17 00:00:00 2001
From: Jacky Yuen <jackyyuen10@gmail.com>
Date: Sat, 10 Jan 2026 23:04:22 +0800
Subject: [PATCH] disp_clk_debug-4

Signed-off-by: Jacky Yuen <jackyyuen10@gmail.com>
---
 drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/disp_al.c | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

diff --git a/drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/disp_al.c b/drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/disp_al.c
index cf76927..748a3b5 100644
--- a/drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/disp_al.c
+++ b/drivers/video/sunxi/disp2/disp/de/lowlevel_v2x/disp_al.c
@@ -664,7 +664,7 @@ static struct lcd_clk_info clk_tbl[] = {
 	{LCD_IF_CPU, 12, 1, 1, 0},
 	{LCD_IF_LVDS, 7, 1, 1, 0},
 #if defined(DSI_VERSION_40)
-	{LCD_IF_DSI, 4, 1, 4, 150000000},
+	{LCD_IF_DSI, 6, 1, 6, 88000000},
 #else
 	{LCD_IF_DSI, 4, 1, 4, 0},
 #endif /*endif DSI_VERSION_40 */
-- 
2.34.1

