#include "ui/controls_guide.h"
#include "platform/platform.h"
#include "ui/text_renderer.h"

void controls_render() {
  int x = 470; /* Right edge, text is right-aligned */
  int y = 175; /* Start above the bottom edge */
  Color headerColor = COLOR_RGB(255, 255, 100); /* Gold */
  Color color = COLOR_RGB(200, 200, 200);       /* Light grey */

  text_draw_string_right(x, y, "CONTROLS", headerColor);
  y += 15;
  text_draw_string_right(x, y, "D-PAD: Rotate", color);
  y += 10;
  text_draw_string_right(x, y, "X: Toggle Backend", color);
  y += 10;
  text_draw_string_right(x, y, "TRI: Change Color", color);
  y += 10;
  text_draw_string_right(x, y, "SQ: Cycle Shape", color);
  y += 10;
  text_draw_string_right(x, y, "CIR: Wire/Solid", color);
  y += 10;
  text_draw_string_right(x, y, "SEL: Exit", color);
}
