#include "stdafx.h"
#include "Win11TaskbarDlg.h"
#include "WindowsSettingHelper.h"

namespace
{
    struct FindRebarParam { HWND rebar{ nullptr }; };
    BOOL CALLBACK FindRebarEnum(HWND hwnd, LPARAM lParam)
    {
        wchar_t cls[64]{};
        if (::GetClassNameW(hwnd, cls, 64) != 0 && wcscmp(cls, L"ReBarWindow32") == 0)
        {
            reinterpret_cast<FindRebarParam*>(lParam)->rebar = hwnd;
            return FALSE;
        }
        return TRUE;
    }
}

void CWin11TaskbarDlg::TryFindTaskbarBandForInsert()
{
    m_hBar = nullptr;
    m_hMin = nullptr;
    FindRebarParam param;
    ::EnumChildWindows(m_hTaskbar, FindRebarEnum, reinterpret_cast<LPARAM>(&param));
    m_hBar = param.rebar;
    if (m_hBar == nullptr)
        m_hBar = ::FindWindowEx(m_hTaskbar, nullptr, L"ReBarWindow32", NULL);
    if (m_hBar == nullptr)
        m_hBar = ::FindWindowEx(m_hTaskbar, nullptr, L"WorkerW", NULL);
    if (m_hBar == nullptr)
        return;
    m_hMin = ::FindWindowEx(m_hBar, nullptr, L"MSTaskSwWClass", NULL);
    if (m_hMin == nullptr)
        m_hMin = ::FindWindowEx(m_hBar, nullptr, L"MSTaskListWClass", NULL);
    if (m_hMin == nullptr)
    {
        m_hBar = nullptr;
        return;
    }
    ::GetWindowRect(m_hMin, m_rcMin);
    ::GetWindowRect(m_hBar, m_rcBar);
    m_left_space = m_rcMin.left - m_rcBar.left;
    m_top_space = m_rcMin.top - m_rcBar.top;
}

void CWin11TaskbarDlg::AdjustTaskbarWndPos(bool force_adjust)
{
    ::GetWindowRect(m_hNotify, m_rcNotify);
    ::GetWindowRect(m_hStart, m_rcStart);
    m_rcStart.MoveToXY(m_rcStart.left - m_rcTaskbar.left, m_rcStart.top - m_rcTaskbar.top);

    //设置窗口大小
    m_rect.right = m_rect.left + m_window_width;
    m_rect.bottom = m_rect.top + m_window_height;

    // 插入模式：缩小任务列表占位，使本窗口占据任务栏实际区域；位置用任务栏客户区坐标，避免 Win11 ReBar 仅覆盖任务列表带导致错位
    if (m_hBar != nullptr && m_hMin != nullptr && ::IsWindow(m_hBar) && ::IsWindow(m_hMin))
    {
        ::GetWindowRect(m_hMin, m_rcMin);
        ::GetWindowRect(m_hBar, m_rcBar);
        if (force_adjust || m_rcMin.Width() != m_last_width)
        {
            m_rcMinOri = m_rcMin;
            m_left_space = m_rcMin.left - m_rcBar.left;
            m_last_width = m_rcMin.Width() - m_rect.Width();
            int new_task_list_width = m_rcMin.Width() - m_rect.Width();
            if (new_task_list_width < 0)
                new_task_list_width = 0;
            if (!theApp.m_taskbar_data.tbar_wnd_on_left || !CWindowsSettingHelper::IsTaskbarCenterAlign())
            {
                ::MoveWindow(m_hMin, m_left_space, 0, new_task_list_width, m_rcMin.Height(), TRUE);
                // 空出区域在任务列表右侧，用任务栏客户区坐标（父窗口为 m_hTaskbar）
                int x_taskbar_client = (m_rcMin.left + new_task_list_width - m_rcTaskbar.left) + 2;
                m_rect.MoveToX(x_taskbar_client);
            }
            else
            {
                ::MoveWindow(m_hMin, m_left_space + m_rect.Width(), 0, new_task_list_width, m_rcMin.Height(), TRUE);
                int x_taskbar_client = m_rcMin.left - m_rcTaskbar.left + 2;
                m_rect.MoveToX(x_taskbar_client);
            }
            m_rect.MoveToY((m_rcTaskbar.Height() - m_rect.Height()) / 2 + DPI(theApp.m_taskbar_data.window_offset_top));
            m_rect.MoveToX(m_rect.left + DPI(theApp.m_taskbar_data.window_offset_left));
            MoveWindow(m_rect);
        }
        return;
    }

    // 叠加模式：按通知区/开始按钮位置摆放，坐标必须为任务栏客户区（父窗口为 m_hTaskbar）
    if (force_adjust || m_rcNotify.Width() != m_last_notify_width || m_rcStart.left != m_last_start_pos)
    {
        m_last_notify_width = m_rcNotify.Width();
        m_last_start_pos = m_rcStart.left;
        if (!theApp.m_taskbar_data.tbar_wnd_on_left || !CWindowsSettingHelper::IsTaskbarCenterAlign())
        {
            // 通知区水平位置：转为任务栏客户区坐标（MoveWindow 使用父窗口客户区）
            int notify_x_client = m_rcNotify.left - m_rcTaskbar.left;
            if (m_rcNotify.left == 0)
            {
                if (m_is_secondary_display)
                    notify_x_client = m_rcTaskbar.Width() - DPI(88);
                else
                    notify_x_client = m_rcTaskbar.Width() - DPI(theApp.m_taskbar_data.taskbar_right_space_win11);
            }
            if (theApp.m_taskbar_data.avoid_overlap_with_widgets && CWindowsSettingHelper::IsTaskbarWidgetsBtnShown() && !CWindowsSettingHelper::IsTaskbarCenterAlign())
                m_rect.MoveToX(notify_x_client - m_rect.Width() + 2 - DPI(theApp.m_taskbar_data.taskbar_left_space_win11));
            else
                m_rect.MoveToX(notify_x_client - m_rect.Width() + 2);
        }
        else
        {
            if (theApp.m_taskbar_data.tbar_wnd_snap)
                m_rect.MoveToX(m_rcStart.left - m_rect.Width() - 2);
            else
            {
                if (CWindowsSettingHelper::IsTaskbarWidgetsBtnShown())
                    m_rect.MoveToX(2 + DPI(theApp.m_taskbar_data.taskbar_left_space_win11));
                else
                    m_rect.MoveToX(2);
            }
        }
        m_rect.MoveToX(m_rect.left + DPI(theApp.m_taskbar_data.window_offset_left));
        m_rect.MoveToY((m_rcStart.Height() - m_rect.Height()) / 2 + (m_rcTaskbar.Height() - m_rcStart.Height()) + DPI(theApp.m_taskbar_data.window_offset_top));
        MoveWindow(m_rect);
    }
}

void CWin11TaskbarDlg::InitTaskbarWnd()
{
    m_hNotify = ::FindWindowEx(m_hTaskbar, 0, L"TrayNotifyWnd", NULL);
    m_hStart = ::FindWindowEx(m_hTaskbar, nullptr, L"Start", NULL);
    ::GetWindowRect(m_hNotify, m_rcNotify);
    TryFindTaskbarBandForInsert();
}

void CWin11TaskbarDlg::ResetTaskbarPos()
{
    if (m_hMin != nullptr && ::IsWindow(m_hMin) && !m_rcMinOri.IsRectEmpty())
        ::MoveWindow(m_hMin, m_left_space, 0, m_rcMinOri.Width(), m_rcMinOri.Height(), TRUE);
}

HWND CWin11TaskbarDlg::GetParentHwnd()
{
    // Win11 下始终以任务栏为父窗口，插入模式时用任务栏客户区坐标定位，避免 ReBar 仅覆盖任务列表带导致显示错位
    return m_hTaskbar;
}

void CWin11TaskbarDlg::CheckTaskbarOnTopOrBottom()
{
    m_taskbar_on_top_or_bottom = true;
}
