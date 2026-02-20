static std::unordered_map<std::string, std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>> proc_callbacks_cn;
static std::unordered_map<HWND, std::string> hwnd_classname_map;
static std::string findClassNameByHwnd(HWND hwnd) {
    auto it = hwnd_classname_map.find(hwnd);
    if (it != hwnd_classname_map.end()) {
        return it->second;
    }
    char c_className[256];
    size_t len = (size_t)GetClassNameA(hwnd, c_className, sizeof(c_className));
    std::string className(c_className, len);
    hwnd_classname_map[hwnd] = className;
	return className;
}
static LRESULT handle_msgbyclassname(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {

    auto it2 = proc_callbacks_cn.find(findClassNameByHwnd(hwnd));
    if (it2 != proc_callbacks_cn.end())
      {
         return it2->second(hwnd, msg, wp, lp);
      }
    return DefWindowProcA(hwnd, msg, wp, lp);
}
static bool safeCall(lua_State* L, bool& isExcept, unsigned long& err) {
	bool islOk = true;
    __try {
        islOk = lua_pcall(L, 4, 1, 0) == LUA_OK;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        isExcept = true;
		err = GetExceptionCode();
    }
	return islOk;
}
static std::unordered_map<HWND, std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>> proc_callbacks_hwnd;

static LRESULT handle_msgbyhwnd(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto it2 = proc_callbacks_hwnd.find(hwnd);
    if (it2 != proc_callbacks_hwnd.end())
    {
		return it2->second(hwnd, msg, wp, lp);
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}


#define REGNAME "winProcRegs"
static std::vector<std::string> classNames;
static std::vector<std::string> menuNames;
static std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> MakeLambda(lua_State* L, int findex)
{
    int ref = addLWinProc(L, findex, REGNAME);
    return [L, ref](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
        if (getLWinProci(L, ref, REGNAME))
        {
            pushWindowStruct(L, HWND, hwnd);
            lua_pushinteger(L, msg);
            lua_pushinteger(L, wp);
            lua_pushinteger(L, lp);

            bool islExcept = false;
            unsigned long err = 0;
            bool islOk = safeCall(L, islExcept, err);
            if (islExcept) {
                lua_getglobal(L, "print");
                lua_pushfstring(L, "Exception 0x%d in WindowProc.", err);
                lua_call(L, 1, 0);
                return DefWindowProc(hwnd, msg, wp, lp);
            }
            if (!islOk) {
                const char* err = lua_tostring(L, -1);
                lua_getglobal(L, "print");
                luaL_traceback(L, L, err, 1);
                lua_call(L, 1, 0);
                lua_pop(L, 1);
                return DefWindowProc(hwnd, msg, wp, lp);
            }

            LRESULT result = (LRESULT)luaL_checkinteger(L, -1);
            lua_pop(L, 1);
            return result;
        }
        else {
			lua_getglobal(L, "print");
			lua_pushfstring(L, "Invalid WindowProc reference. %d", ref);
			return DefWindowProc(hwnd, msg, wp, lp);
        }
        };
}
static LRESULT(*ToWindowProc(lua_State* L, HWND hwnd, int findex))(HWND, UINT, WPARAM, LPARAM)
{
    proc_callbacks_hwnd[hwnd] = MakeLambda(L, findex);
    return handle_msgbyhwnd;
}
static LRESULT(*ToWindowProc(lua_State* L, std::string className, int findex))(HWND, UINT, WPARAM, LPARAM)
{
    
    proc_callbacks_cn[className] = MakeLambda(L, findex);
    return handle_msgbyclassname;
}


static void table_to_wndclassexa(lua_State* L, int index, WNDCLASSEXA* wc)
{
    ZeroMemory(wc, sizeof(WNDCLASSEXA));
    wc->cbSize = sizeof(WNDCLASSEXA);
    if (!lua_istable(L, index)) return;

    index = lua_absindex(L, index);
    lua_getfield(L, index, "hInstance");
    if (lua_isuserdata(L, -1))
        wc->hInstance = (HINSTANCE)lua_touserdata(L, -1);
    else
        wc->hInstance = GetModuleHandleA(NULL);
    lua_pop(L, 1);
    // lpszClassName (string)
    lua_getfield(L, index, "lpszClassName");
        classNames.emplace_back(luaL_checkstring(L, -1));
        wc->lpszClassName = classNames.back().c_str();

    lua_pop(L, 1);
    lua_getfield(L, index, "lpfnWndProc");
    if (lua_isfunction(L, -1))
    {
        wc->lpfnWndProc = ToWindowProc(L, wc->lpszClassName, -1);
    }
    else if (lua_isuserdata(L, -1))
    {
        wc->lpfnWndProc = (WNDPROC)lua_touserdata(L, -1);
    }
	else
    {
        wc->lpfnWndProc = DefWindowProc;
        lua_pop(L, 1);
    }
    lua_getfield(L, index, "style");
    if (lua_isinteger(L, -1))
        wc->style = (UINT)lua_tointeger(L, -1);
    else
        wc->style = CS_HREDRAW | CS_VREDRAW;
    lua_pop(L, 1);
    lua_getfield(L, index, "cbClsExtra");
    wc->cbClsExtra = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, index, "cbWndExtra");
    wc->cbWndExtra = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, index, "hIcon");
    if (lua_isuserdata(L, -1))
        wc->hIcon = (HICON)lua_touserdata(L, -1);
    else
        wc->hIcon = NULL;
    lua_pop(L, 1);

    lua_getfield(L, index, "hCursor");
    if (lua_isuserdata(L, -1))
        wc->hCursor = (HCURSOR)lua_touserdata(L, -1);
    else
        wc->hCursor = LoadCursorA(NULL, IDC_ARROW);
    lua_pop(L, 1);

    lua_getfield(L, index, "hbrBackground");
    if (lua_isuserdata(L, -1))
        wc->hbrBackground = (HBRUSH)lua_touserdata(L, -1);
    else if (lua_isinteger(L, -1)) {
        wc->hbrBackground = (HBRUSH)lua_tointeger(L, -1);
    }

    else
        wc->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    lua_pop(L, 1);

    lua_getfield(L, index, "lpszMenuName");
    if (lua_isstring(L, -1))
    {
        menuNames.emplace_back(luaL_checkstring(L, -1));
        wc->lpszMenuName = menuNames.back().c_str();
    }
    else
        wc->lpszMenuName = NULL;
    lua_pop(L, 1);
}
Lua_Function(DefWindowProc)
{
    HWND hWnd = luaL_wingetbycheckudata(L, 1, HWND);
    UINT msg = (UINT)luaL_checkinteger(L, 2);
    WPARAM wParam = (WPARAM)luaL_checkinteger(L, 3);
    LPARAM lParam = (LPARAM)luaL_checkinteger(L, 4);
    lua_pushinteger(L, DefWindowProcA(hWnd, msg, wParam, lParam));
    return 1;
}
Lua_Function(ToWindowProc)
{
    HWND hwnd = luaL_wingetbycheckudata(L, 1, HWND);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushlightuserdata(L, ToWindowProc(L, hwnd, 2));
    return 1;
}
Lua_Function(CallWindowProc)
{
    WNDPROC winProc = (WNDPROC)lua_touserdata(L, 1);
    HWND hWnd = luaL_wingetbycheckudata(L, 2, HWND);
    UINT msg = (UINT)luaL_checkinteger(L, 3);
    WPARAM wParam = (WPARAM)luaL_checkinteger(L, 4);
    LPARAM lParam = (LPARAM)luaL_checkinteger(L, 5);
    lua_pushinteger(L, CallWindowProcA(winProc, hWnd, msg, wParam, lParam));
    return 1;
}
Lua_Function(RegisterClassEx)
{
    WNDCLASSEXA wc;
    table_to_wndclassexa(L, 1, &wc);

    ATOM atom = RegisterClassExA(&wc);
    if (atom == 0)
    {
        DWORD err = GetLastError();
        lua_pushnil(L);
        lua_pushinteger(L, err);
        return 2;
    }
    lua_pushinteger(L, (lua_Integer)atom);
    return 1;
}
static void PushMSGToLua(lua_State* L, const LPMSG msg, int tablerindex) {
    int tablei = lua_absindex(L, tablerindex);

    lua_pushstring(L, "hwnd");
    pushWindowStruct(L, HWND, msg->hwnd);
    lua_settable(L, tablei);

    lua_pushstring(L, "message");
    lua_pushinteger(L, msg->message);
    lua_settable(L, tablei);

    lua_pushstring(L, "wParam");
    lua_pushinteger(L, msg->wParam);
    lua_settable(L, tablei);

    lua_pushstring(L, "lParam");
    lua_pushinteger(L, msg->lParam);
    lua_settable(L, tablei);

    lua_pushstring(L, "time");
    lua_pushinteger(L, msg->time);
    lua_settable(L, tablei);

    lua_pushstring(L, "pt");
    lua_newtable(L);

    lua_pushstring(L, "x");
    lua_pushinteger(L, msg->pt.x);
    lua_settable(L, -3);

    lua_pushstring(L, "y");
    lua_pushinteger(L, msg->pt.y);
    lua_settable(L, -3);

    lua_settable(L, tablei);

#ifdef _MAC
    lua_pushstring(L, "lPrivate");
    lua_pushinteger(L, msg->lPrivate);
    lua_settable(L, tablei);
#endif
}

static bool GetMSGFromLua(lua_State* L, int index, LPMSG out) {
    if (!lua_istable(L, index)) return false;

    lua_getfield(L, index, "hwnd");
    out->hwnd = (HWND)(uintptr_t)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "message");
    out->message = (UINT)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "wParam");
    out->wParam = (WPARAM)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "lParam");
    out->lParam = (LPARAM)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "time");
    out->time = (DWORD)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "pt");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "x");
        out->pt.x = (LONG)lua_tointeger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "y");
        out->pt.y = (LONG)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
#ifdef _MAC
    lua_getfield(L, index, "lPrivate");
    out->lPrivate = (DWORD)lua_tointeger(L, -1);
    lua_pop(L, 1);
#endif
    return true;
}
Lua_Function(GetMessage)
{
    MSG msg;
	luaL_checktype(L, 1, LUA_TTABLE);
    HWND hWnd = luaL_wingetbyudata(L, 2, HWND);
    UINT wMsgFilterMin = (UINT)luaL_optinteger(L, 3, 0);
    UINT wMsgFilterMax = (UINT)luaL_optinteger(L, 4, 0);
    BOOL result = GetMessageA(&msg, hWnd, wMsgFilterMin, wMsgFilterMax);
    PushMSGToLua(L, &msg, 1);
    lua_pushboolean(L, result > 0); 
    return 1;
}

Lua_Function(TranslateMessage)
{
    MSG msg;
    if (!GetMSGFromLua(L, 1, &msg)) {
        return luaL_error(L, "Expected table representing MSG as argument #1");
    }

    BOOL result = TranslateMessage(&msg);
    lua_pushboolean(L, result);
    return 1;
}

Lua_Function(DispatchMessage)
{
    MSG msg;
    if (!GetMSGFromLua(L, 1, &msg)) {
        return luaL_error(L, "Expected table representing MSG as argument #1");
    }

    LRESULT result = DispatchMessageA(&msg);
    lua_pushinteger(L, result);
    return 1;
}
Lua_Function(PostQuitMessage) {
    int exitCode = (int)luaL_optinteger(L, 1, 0);
    PostQuitMessage(exitCode);
    return 0; 
}

Lua_Function(PeekMessage)
{
    MSG msg;
    HWND hwnd = NULL;
    UINT wMsgFilterMin = 0;
    UINT wMsgFilterMax = 0;
    UINT wRemoveMsg = PM_REMOVE;
	luaL_checktype(L, 1, LUA_TTABLE);
    hwnd = luaL_wingetbyudata(L, 2, HWND);
    wMsgFilterMin = (UINT)luaL_checkinteger(L, 3);
    wMsgFilterMax = (UINT)luaL_checkinteger(L, 4);
    wRemoveMsg = (UINT)luaL_checkinteger(L, 5);

    BOOL hasMessage = PeekMessageA(&msg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    if (hasMessage) {
        PushMSGToLua(L, &msg, 1);
	}
	lua_pushboolean(L, hasMessage);
    
    return 1;
}
Lua_Function(LoopMessages)
{
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessageA(&msg, NULL, 0, 0)) != 0) {
        if (bRet == -1) {
			lua_pushboolean(L, false);
            lua_pushinteger(L, GetLastError());
            return 2;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    lua_pushboolean(L, true);
	return 1;
}
Lua_Function(PostMessage)
{
    HWND hWnd = luaL_wingetbycheckudata(L, 1, HWND);
    UINT Msg = (UINT)luaL_checkinteger(L, 2);
    WPARAM wParam = (WPARAM)luaL_optinteger(L, 3, 0);
    LPARAM lParam = (LPARAM)luaL_optinteger(L, 4, 0);
    BOOL res = PostMessageA(hWnd, Msg, wParam, lParam);
    lua_pushboolean(L, res);
    return 1;
}
void register_all_wm_messages(lua_State* L) {
    REGIMACRO(WM_NULL)
        REGIMACRO(WM_CREATE)
        REGIMACRO(WM_DESTROY)
        REGIMACRO(WM_MOVE)
        REGIMACRO(WM_SIZE)
        REGIMACRO(WM_ACTIVATE)
        REGIMACRO(WM_SETFOCUS)
        REGIMACRO(WM_KILLFOCUS)
        REGIMACRO(WM_ENABLE)
        REGIMACRO(WM_SETREDRAW)
        REGIMACRO(WM_SETTEXT)
        REGIMACRO(WM_GETTEXT)
        REGIMACRO(WM_GETTEXTLENGTH)
        REGIMACRO(WM_PAINT)
        REGIMACRO(WM_CLOSE)
        REGIMACRO(WM_QUERYENDSESSION)
        REGIMACRO(WM_QUIT)
        REGIMACRO(WM_QUERYOPEN)
        REGIMACRO(WM_ERASEBKGND)
        REGIMACRO(WM_SYSCOLORCHANGE)
        REGIMACRO(WM_ENDSESSION)
        REGIMACRO(WM_SHOWWINDOW)
        REGIMACRO(WM_WININICHANGE)
        REGIMACRO(WM_SETTINGCHANGE)
        REGIMACRO(WM_DEVMODECHANGE)
        REGIMACRO(WM_ACTIVATEAPP)
        REGIMACRO(WM_FONTCHANGE)
        REGIMACRO(WM_TIMECHANGE)
        REGIMACRO(WM_CANCELMODE)
        REGIMACRO(WM_SETCURSOR)
        REGIMACRO(WM_MOUSEACTIVATE)
        REGIMACRO(WM_CHILDACTIVATE)
        REGIMACRO(WM_QUEUESYNC)
        REGIMACRO(WM_GETMINMAXINFO)
        REGIMACRO(WM_PAINTICON)
        REGIMACRO(WM_ICONERASEBKGND)
        REGIMACRO(WM_NEXTDLGCTL)
        REGIMACRO(WM_SPOOLERSTATUS)
        REGIMACRO(WM_DRAWITEM)
        REGIMACRO(WM_MEASUREITEM)
        REGIMACRO(WM_DELETEITEM)
        REGIMACRO(WM_VKEYTOITEM)
        REGIMACRO(WM_CHARTOITEM)
        REGIMACRO(WM_SETFONT)
        REGIMACRO(WM_GETFONT)
        REGIMACRO(WM_SETHOTKEY)
        REGIMACRO(WM_GETHOTKEY)
        REGIMACRO(WM_QUERYDRAGICON)
        REGIMACRO(WM_COMPAREITEM)
        REGIMACRO(WM_GETOBJECT)
        REGIMACRO(WM_COMPACTING)
        REGIMACRO(WM_COMMNOTIFY)
        REGIMACRO(WM_WINDOWPOSCHANGING)
        REGIMACRO(WM_WINDOWPOSCHANGED)
        REGIMACRO(WM_POWER)
        REGIMACRO(WM_COPYDATA)
        REGIMACRO(WM_CANCELJOURNAL)
        REGIMACRO(WM_NOTIFY)
        REGIMACRO(WM_INPUTLANGCHANGEREQUEST)
        REGIMACRO(WM_INPUTLANGCHANGE)
        REGIMACRO(WM_TCARD)
        REGIMACRO(WM_HELP)
        REGIMACRO(WM_USERCHANGED)
        REGIMACRO(WM_NOTIFYFORMAT)
        REGIMACRO(WM_CONTEXTMENU)
        REGIMACRO(WM_STYLECHANGING)
        REGIMACRO(WM_STYLECHANGED)
        REGIMACRO(WM_DISPLAYCHANGE)
        REGIMACRO(WM_GETICON)
        REGIMACRO(WM_SETICON)
        REGIMACRO(WM_NCCREATE)
        REGIMACRO(WM_NCDESTROY)
        REGIMACRO(WM_NCCALCSIZE)
        REGIMACRO(WM_NCHITTEST)
        REGIMACRO(WM_NCPAINT)
        REGIMACRO(WM_NCACTIVATE)
        REGIMACRO(WM_GETDLGCODE)
        REGIMACRO(WM_SYNCPAINT)
        REGIMACRO(WM_NCMOUSEMOVE)
        REGIMACRO(WM_NCLBUTTONDOWN)
        REGIMACRO(WM_NCLBUTTONUP)
        REGIMACRO(WM_NCLBUTTONDBLCLK)
        REGIMACRO(WM_NCRBUTTONDOWN)
        REGIMACRO(WM_NCRBUTTONUP)
        REGIMACRO(WM_NCRBUTTONDBLCLK)
        REGIMACRO(WM_NCMBUTTONDOWN)
        REGIMACRO(WM_NCMBUTTONUP)
        REGIMACRO(WM_NCMBUTTONDBLCLK)
        REGIMACRO(WM_KEYDOWN)
        REGIMACRO(WM_KEYUP)
        REGIMACRO(WM_CHAR)
        REGIMACRO(WM_DEADCHAR)
        REGIMACRO(WM_SYSKEYDOWN)
        REGIMACRO(WM_SYSKEYUP)
        REGIMACRO(WM_SYSCHAR)
        REGIMACRO(WM_SYSDEADCHAR)
        REGIMACRO(WM_UNICHAR)
        REGIMACRO(WM_IME_STARTCOMPOSITION)
        REGIMACRO(WM_IME_ENDCOMPOSITION)
        REGIMACRO(WM_IME_COMPOSITION)
        REGIMACRO(WM_IME_KEYLAST)
        REGIMACRO(WM_INITDIALOG)
        REGIMACRO(WM_COMMAND)
        REGIMACRO(WM_SYSCOMMAND)
        REGIMACRO(WM_TIMER)
        REGIMACRO(WM_HSCROLL)
        REGIMACRO(WM_VSCROLL)
        REGIMACRO(WM_INITMENU)
        REGIMACRO(WM_INITMENUPOPUP)
        REGIMACRO(WM_MENUSELECT)
        REGIMACRO(WM_MENUCHAR)
        REGIMACRO(WM_ENTERIDLE)
        REGIMACRO(WM_MENURBUTTONUP)
        REGIMACRO(WM_MENUDRAG)
        REGIMACRO(WM_MENUGETOBJECT)
        REGIMACRO(WM_UNINITMENUPOPUP)
        REGIMACRO(WM_MENUCOMMAND)
        REGIMACRO(WM_CHANGEUISTATE)
        REGIMACRO(WM_UPDATEUISTATE)
        REGIMACRO(WM_QUERYUISTATE)
        REGIMACRO(WM_CTLCOLORMSGBOX)
        REGIMACRO(WM_CTLCOLOREDIT)
        REGIMACRO(WM_CTLCOLORLISTBOX)
        REGIMACRO(WM_CTLCOLORBTN)
        REGIMACRO(WM_CTLCOLORDLG)
        REGIMACRO(WM_CTLCOLORSCROLLBAR)
        REGIMACRO(WM_CTLCOLORSTATIC)
        REGIMACRO(WM_MOUSEMOVE)
        REGIMACRO(WM_LBUTTONDOWN)
        REGIMACRO(WM_LBUTTONUP)
        REGIMACRO(WM_LBUTTONDBLCLK)
        REGIMACRO(WM_RBUTTONDOWN)
        REGIMACRO(WM_RBUTTONUP)
        REGIMACRO(WM_RBUTTONDBLCLK)
        REGIMACRO(WM_MBUTTONDOWN)
        REGIMACRO(WM_MBUTTONUP)
        REGIMACRO(WM_MBUTTONDBLCLK)
        REGIMACRO(WM_MOUSEWHEEL)
        REGIMACRO(WM_XBUTTONDOWN)
        REGIMACRO(WM_XBUTTONUP)
        REGIMACRO(WM_PARENTNOTIFY)
        REGIMACRO(WM_ENTERMENULOOP)
        REGIMACRO(WM_EXITMENULOOP)
        REGIMACRO(WM_NEXTMENU)
        REGIMACRO(WM_SIZING)
        REGIMACRO(WM_CAPTURECHANGED)
        REGIMACRO(WM_MOVING)
        REGIMACRO(WM_POWERBROADCAST)
        REGIMACRO(WM_DEVICECHANGE)
        REGIMACRO(WM_MDICREATE)
        REGIMACRO(WM_MDIDESTROY)
        REGIMACRO(WM_MDIACTIVATE)
        REGIMACRO(WM_MDIRESTORE)
        REGIMACRO(WM_MDINEXT)
        REGIMACRO(WM_MDIMAXIMIZE)
        REGIMACRO(WM_MDITILE)
        REGIMACRO(WM_MDICASCADE)
        REGIMACRO(WM_MDIICONARRANGE)
        REGIMACRO(WM_MDIGETACTIVE)
        REGIMACRO(WM_MDISETMENU)
        REGIMACRO(WM_ENTERSIZEMOVE)
        REGIMACRO(WM_EXITSIZEMOVE)
        REGIMACRO(WM_DROPFILES)
        REGIMACRO(WM_MDIREFRESHMENU)
        REGIMACRO(WM_IME_SETCONTEXT)
        REGIMACRO(WM_IME_NOTIFY)
        REGIMACRO(WM_IME_CONTROL)
        REGIMACRO(WM_IME_COMPOSITIONFULL)
        REGIMACRO(WM_IME_SELECT)
        REGIMACRO(WM_IME_CHAR)
        REGIMACRO(WM_IME_REQUEST)
        REGIMACRO(WM_IME_KEYDOWN)
        REGIMACRO(WM_IME_KEYUP)
        REGIMACRO(WM_MOUSEHOVER)
        REGIMACRO(WM_MOUSELEAVE)
        REGIMACRO(WM_CUT)
        REGIMACRO(WM_COPY)
        REGIMACRO(WM_PASTE)
        REGIMACRO(WM_CLEAR)
        REGIMACRO(WM_UNDO)
        REGIMACRO(WM_RENDERFORMAT)
        REGIMACRO(WM_RENDERALLFORMATS)
        REGIMACRO(WM_DESTROYCLIPBOARD)
        REGIMACRO(WM_DRAWCLIPBOARD)
        REGIMACRO(WM_PAINTCLIPBOARD)
        REGIMACRO(WM_VSCROLLCLIPBOARD)
        REGIMACRO(WM_SIZECLIPBOARD)
        REGIMACRO(WM_ASKCBFORMATNAME)
        REGIMACRO(WM_CHANGECBCHAIN)
        REGIMACRO(WM_HSCROLLCLIPBOARD)
        REGIMACRO(WM_MOUSEFIRST)
        REGIMACRO(WM_MOUSELAST)
        REGIMACRO(WM_QUEUESYNC)
        REGIMACRO(WM_GETMINMAXINFO)
        REGIMACRO(WM_ICONERASEBKGND)
        REGIMACRO(WM_NEXTDLGCTL)
        REGIMACRO(WM_SPOOLERSTATUS)
        REGIMACRO(WM_DRAWITEM)
        REGIMACRO(WM_MEASUREITEM)
        REGIMACRO(WM_DELETEITEM)
        REGIMACRO(WM_VKEYTOITEM)
        REGIMACRO(WM_CHARTOITEM)
        REGIMACRO(WM_SETFONT)
        REGIMACRO(WM_GETFONT)
        REGIMACRO(WM_SETHOTKEY)
        REGIMACRO(WM_GETHOTKEY)
        REGIMACRO(WM_QUERYDRAGICON)
        REGIMACRO(WM_COMPAREITEM)
        REGIMACRO(WM_GETOBJECT)
        REGIMACRO(WM_COMPACTING)
        REGIMACRO(WM_COMMNOTIFY)
        REGIMACRO(WM_WINDOWPOSCHANGING)
        REGIMACRO(WM_WINDOWPOSCHANGED)
        REGIMACRO(WM_POWERBROADCAST)
        REGIMACRO(WM_COPYDATA)
            REGIMACRO(BS_PUSHBUTTON)
            REGIMACRO(BS_DEFPUSHBUTTON)
            REGIMACRO(BS_CHECKBOX)
            REGIMACRO(BS_AUTOCHECKBOX)
            REGIMACRO(BS_RADIOBUTTON)
            REGIMACRO(BS_3STATE)
            REGIMACRO(BS_AUTO3STATE)
            REGIMACRO(BS_GROUPBOX)
            REGIMACRO(BS_USERBUTTON)
            REGIMACRO(BS_AUTORADIOBUTTON)
            REGIMACRO(BS_PUSHBOX)
            REGIMACRO(BS_OWNERDRAW)
            REGIMACRO(BS_TYPEMASK)
            REGIMACRO(BS_LEFTTEXT)
            REGIMACRO(BS_TEXT)
            REGIMACRO(BS_ICON)
            REGIMACRO(BS_BITMAP)
            REGIMACRO(BS_LEFT)
            REGIMACRO(BS_RIGHT)
            REGIMACRO(BS_CENTER)
            REGIMACRO(BS_TOP)
            REGIMACRO(BS_BOTTOM)
            REGIMACRO(BS_VCENTER)
            REGIMACRO(BS_PUSHLIKE)
            REGIMACRO(BS_MULTILINE)
            REGIMACRO(BS_NOTIFY)
            REGIMACRO(BS_FLAT)
            REGIMACRO(BS_RIGHTBUTTON)

            REGIMACRO(BN_CLICKED)
            REGIMACRO(BN_PAINT)
            REGIMACRO(BN_HILITE)
            REGIMACRO(BN_UNHILITE)
            REGIMACRO(BN_DISABLE)
            REGIMACRO(BN_DOUBLECLICKED)
            REGIMACRO(BN_PUSHED)
            REGIMACRO(BN_UNPUSHED)
            REGIMACRO(BN_DBLCLK)
            REGIMACRO(BN_SETFOCUS)
            REGIMACRO(BN_KILLFOCUS)

            REGIMACRO(BM_GETCHECK)
            REGIMACRO(BM_SETCHECK)
            REGIMACRO(BM_GETSTATE)
            REGIMACRO(BM_SETSTATE)
            REGIMACRO(BM_SETSTYLE)
            REGIMACRO(BM_CLICK)
            REGIMACRO(BM_GETIMAGE)
            REGIMACRO(BM_SETIMAGE)
            REGIMACRO(BM_SETDONTCLICK)

            REGIMACRO(BST_UNCHECKED)
            REGIMACRO(BST_CHECKED)
            REGIMACRO(BST_INDETERMINATE)
            REGIMACRO(BST_PUSHED)
            REGIMACRO(BST_FOCUS)
}
