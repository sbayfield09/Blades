#include "blades.h"
const char *const blade_names[BLADE_COUNT]={"Marketplace","Xbox Live","Games","Media","System"};
/* Spring-2007 five-blade target; 5766 is visible in the supplied recording. */
static const char *const roots[BLADE_COUNT][8]={
    {"Game Store","Video Store","Account Management",0},
    {"Messages","Friends","Chat and IM","Open Tray",0},
    {"Achievements","Played Games","Xbox Live Arcade","Demos and More","Open Tray",0},
    {"Music","Pictures","Video","Video Marketplace","Media Center","Open Tray"},
    {"Console Settings","Family Settings","Memory","Network Settings","Computers","Initial Setup","Update Blades"}
};
static const char *const children[BLADE_COUNT][7][5]={
    {{"Game downloads support"},{"Video downloads support"},{"Account services support"}},
    {{"No messages"},{"No friends"},{"Chat and instant messaging support"},{"Disc tray control is not implemented"}},
    {{"Achievements support"},{"Game Library","Apps","Homebrew","File Manager","Scan Folders"},{"Arcade library support"},{"Demo library support"},{"Disc tray control is not implemented"}},
    {{"Browse music","Playback support"},{"Browse pictures","Image decoder support"},{"Jellyfin","Local"},{"Video downloads support"},{"Server Address","Username","Password","Connect","Remove Connection"},{"Disc tray control is not implemented"}},
    {{"Display: 1280 x 720","Audio","Language","About Blades"},{"Family settings support"},{"Storage enumeration support"},{"Network information"},{"FTP server"},{"Initial setup support"},{"Check for Updates","Download Update"}}
};
int menu_count(int b,int depth,int selected) {
    int n=0;
    if(!depth) {while(n<8&&roots[b][n])n++;}
    else {while(n<5&&children[b][selected][n])n++;}
    return n;
}
const char *menu_label(int b,int depth,int selected,int row) {
    return depth?children[b][selected][row]:roots[b][row];
}
bool blade_has_profile(int blade) {return blade==BLADE_LIVE||blade==BLADE_GAMES||blade==BLADE_MEDIA;}
bool blade_has_tray(int blade) {return blade_has_profile(blade);}
