#pragma once
#define Lua_Function(name) int ll_##name(lua_State* L)
#define luaL_wingetbycheckudata(L, idx, type) \
    *(type*)luaL_checkuserdata(L, idx)
#define pushWindowStruct(L,clazz, val) lua_pushlightuserdata(L, val)
#define INIT_LUAOPEN() extern "C" __declspec(dllexport) int luaopen_lwindows(lua_State* L){
#define END_LUAOPEN() return 0;}
#define REGSTRUCT(T) registerNewStruct(L, ##T##);
#define REGVALUEMACRO(name,type) (lua_push##type(L, ##name##), lua_setglobal(L, #name));
#define REGIMACRO(N) REGVALUEMACRO(##N##, integer)
#define REGSMACRO(S) REGVALUEMACRO(##S##, string)
#define REGUMACRO(T, v) (pushWindowStruct(L,##T##,##v##), lua_setglobal(L, #v));
#define INIT_WPR() luaL_Reg winapi_reg[] = {
#define ADD2WPR(name) {#name, ll_##name},
#define END_WPR() {NULL,NULL}};REGISTER_WPR()
#define REGISTER_WPR() (lua_pushglobaltable(L),luaL_setfuncs(L, winapi_reg, 0));
#define REGISTERINH(name) Lua_Function(##name##);
#define luaL_checkuserdata(L, idx) \
    (luaL_checktype(L, idx, LUA_TUSERDATA), lua_touserdata(L, idx))
#define luaL_checklightuserdata(L, idx) \
    (luaL_checktype(L, idx, LUA_TLIGHTUSERDATA), lua_touserdata(L, idx))
#define luaL_wingetbyudata(L, idx, type) ([](lua_State* L_, int i) -> type { \
    void* p = lua_touserdata(L_, i);                                       \
    return p ? *(type*)p : NULL;                                           \
})(L, idx)
bool removeLWinProc(lua_State* L, lua_Integer i, const std::string& regName);
bool getLWinProci(lua_State* L, lua_Integer i, const std::string& regName);
int addLWinProc(lua_State* L, int index, const std::string& regName);
void table_toLPRECT(lua_State* L, int index, LPRECT prc);
void LPRECT_to_table(lua_State* L, const LPRECT prc, int tindex = 0);
SECURITY_ATTRIBUTES table2SECURITY_ATTRIBUTES(lua_State* L, int index);
void SECURITY_ATTRIBUTTES2table(lua_State* L, int index, const SECURITY_ATTRIBUTES& sa);