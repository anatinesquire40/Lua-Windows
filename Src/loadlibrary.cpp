Lua_Function(LoadLibrary)
{
    LPCSTR name = (LPCSTR)luaL_checkstring(L, 1);
    pushWindowStruct(L, HMODULE, LoadLibraryA(name));
    return 1;
}
Lua_Function(FreeLibrary)
{
    HMODULE mod = luaL_wingetbycheckudata(L, 1, HMODULE);
    BOOL suc = FreeLibrary(mod);
    lua_pushboolean(L, suc);
    return 1;
}
Lua_Function(GetProcAddress)
{
    HMODULE hMod = luaL_wingetbycheckudata(L, 1, HMODULE);
    const char* funcName = luaL_checkstring(L, 2);

    if (!hMod || !funcName)
        return luaL_error(L, "Invalid module handle or function name");
    FARPROC func = GetProcAddress(hMod, funcName);
    lua_pushlightuserdata(L, (void*)func);
    return 1;
}
Lua_Function(GetModuleHandleEx)
{
    DWORD dwFlags = (DWORD)lua_tointeger(L, 1);
    LPCSTR lpModuleName = luaL_optstring(L, 2, NULL);

    HMODULE* hModule = (HMODULE*)lua_touserdata(L, 3);
    BOOL res = GetModuleHandleExA(dwFlags, lpModuleName, hModule);

    lua_pushboolean(L, res);
    return 1;
}