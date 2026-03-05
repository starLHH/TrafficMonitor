#pragma once
#include "TaskBarDlg.h"
class CWin11TaskbarDlg :
    public CTaskBarDlg
{
public:

private:
    // 通过 CTaskBarDlg 继承
    void InitTaskbarWnd() override;
    virtual void AdjustTaskbarWndPos(bool force_adjust) override;
    void ResetTaskbarPos() override;
    virtual HWND GetParentHwnd() override;

    /** 尝试查找 ReBar 与任务列表，若找到则使用“插入”模式（缩小任务列表占位，不遮挡应用按钮） */
    void TryFindTaskbarBandForInsert();

    HWND m_hNotify;     //任务栏通知区域的句柄
    HWND m_hStart;      //开始按钮的句柄
    CRect m_rcNotify;   //任务栏通知区域的矩形区域
    CRect m_rcStart;     //开始按钮的矩形区域
    int m_last_notify_width{};
    int m_last_start_pos{};

    // 插入模式：与经典任务栏相同，通过缩小任务列表窗口为当前窗口腾出实际区域
    HWND m_hBar{ nullptr };     // ReBarWindow32 句柄，非空表示使用插入模式
    HWND m_hMin{ nullptr };     // 任务列表（MSTaskSwWClass/MSTaskListWClass）句柄
    CRect m_rcBar;
    CRect m_rcMin;
    CRect m_rcMinOri;   // 程序退出时恢复任务列表用
    int m_left_space{};
    int m_top_space{};
    int m_last_width{ -1 };
    int m_last_height{ -1 };

    // 通过 CTaskBarDlg 继承
    void CheckTaskbarOnTopOrBottom() override;

};

