require"lwindows"
function CreateWindow(...)
    return CreateWindowEx(nil, ...)
end
MessageBox = MessageBoxEx
function GetModuleHandle(mod)
    local hmod = Val2Addr(0)
    GetModuleHandleEx(nil, mod, hmod)
    return hmod
end
Sleep = SleepEx
DrawText = DrawTextEx