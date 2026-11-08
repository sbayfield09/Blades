#ifndef THEME_H
#define THEME_H
#include "blades.h"
void theme_label_fit(Canvas *c,int x,int y,int size,int width,const char *label);
uint32_t theme_mix(uint32_t a,uint32_t b,float amount);
uint32_t theme_color(int blade);
int theme_left(int blade);
int theme_right(int blade);
int theme_content_left(int blade);
enum { SUBMENU_LEFT=94, SUBMENU_RIGHT=1165, SUBMENU_CONTENT=154, SUBMENU_RAIL_LEFT=24 };
float theme_rail_position(int blade,int rail);
void theme_background(Canvas *c,int blade,float time);
void theme_frame(Canvas *c,int blade,float depth,float header,float footer);
void theme_leaf(Canvas *c,int from,int to,float progress);
void theme_submenu_chrome(Canvas *c,int blade,float depth,bool closing);
void theme_chrome(Canvas *c,int blade,float position);
void theme_icon(Canvas *c,int x,int y,int kind,uint32_t color);
void theme_label(Canvas *c,int x,int y,int size,const char *label,bool selected);
void theme_menu_shadow(Canvas *c,int x,int top,int width,int bottom,bool rounded);
void theme_menu_upper_shadow(Canvas *c,int x,int first,int width,int selected_top,bool rounded);
void theme_round(Canvas *c,int x,int y,int w,int h,int radius,uint32_t color);
void theme_outline(Canvas *c,int x,int y,int w,int h,int radius,uint32_t color);
#endif
