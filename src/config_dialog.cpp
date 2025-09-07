#include "../include/foo_albumart_grid.h"
#include <resource.h>

// Configuration dialog implementation
BOOL ConfigDialog::OnInitDialog(HWND hwndFocus, LPARAM lParam) {
    // Initialize grouping mode combo
    HWND hGroupingCombo = GetDlgItem(IDC_GROUPING_COMBO);
    if (hGroupingCombo) {
        SendMessage(hGroupingCombo, CB_ADDSTRING, 0, (LPARAM)L"By Album");
        SendMessage(hGroupingCombo, CB_ADDSTRING, 0, (LPARAM)L"By Artist"); 
        SendMessage(hGroupingCombo, CB_ADDSTRING, 0, (LPARAM)L"By Genre");
        SendMessage(hGroupingCombo, CB_ADDSTRING, 0, (LPARAM)L"By Year");
        SendMessage(hGroupingCombo, CB_ADDSTRING, 0, (LPARAM)L"By Folder");
        SendMessage(hGroupingCombo, CB_SETCURSEL, (WPARAM)m_config.grouping_mode, 0);
    }
    
    // Initialize grid size combo
    HWND hSizeCombo = GetDlgItem(IDC_SIZE_COMBO);
    if (hSizeCombo) {
        SendMessage(hSizeCombo, CB_ADDSTRING, 0, (LPARAM)L"Small (128px)");
        SendMessage(hSizeCombo, CB_ADDSTRING, 0, (LPARAM)L"Medium (192px)");
        SendMessage(hSizeCombo, CB_ADDSTRING, 0, (LPARAM)L"Large (256px)");
        
        int size_index = 1; // Default to medium
        if (m_config.grid_size == GridSize::SMALL) size_index = 0;
        else if (m_config.grid_size == GridSize::LARGE) size_index = 2;
        
        SendMessage(hSizeCombo, CB_SETCURSEL, size_index, 0);
    }
    
    // Initialize checkboxes
    CheckDlgButton(IDC_THEME_CHECK, m_config.dark_theme ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_OVERLAY_CHECK, m_config.show_text_overlay ? BST_CHECKED : BST_UNCHECKED);
    
    // Initialize cache size edit
    HWND hCacheEdit = GetDlgItem(IDC_CACHE_EDIT);
    if (hCacheEdit) {
        wchar_t cache_str[32];
        swprintf_s(cache_str, L"%d", m_config.cache_size_mb);
        SetWindowText(hCacheEdit, cache_str);
    }
    
    return TRUE;
}

void ConfigDialog::OnOK(UINT uNotifyCode, int nID, HWND hWndCtl) {
    // Get grouping mode
    HWND hGroupingCombo = GetDlgItem(IDC_GROUPING_COMBO);
    if (hGroupingCombo) {
        int sel = (int)SendMessage(hGroupingCombo, CB_GETCURSEL, 0, 0);
        if (sel != CB_ERR) {
            m_config.grouping_mode = (GroupingMode)sel;
        }
    }
    
    // Get grid size
    HWND hSizeCombo = GetDlgItem(IDC_SIZE_COMBO);
    if (hSizeCombo) {
        int sel = (int)SendMessage(hSizeCombo, CB_GETCURSEL, 0, 0);
        if (sel != CB_ERR) {
            switch (sel) {
            case 0: m_config.grid_size = GridSize::SMALL; break;
            case 1: m_config.grid_size = GridSize::MEDIUM; break;
            case 2: m_config.grid_size = GridSize::LARGE; break;
            }
        }
    }
    
    // Get checkboxes
    m_config.dark_theme = (IsDlgButtonChecked(IDC_THEME_CHECK) == BST_CHECKED);
    m_config.show_text_overlay = (IsDlgButtonChecked(IDC_OVERLAY_CHECK) == BST_CHECKED);
    
    // Get cache size
    HWND hCacheEdit = GetDlgItem(IDC_CACHE_EDIT);
    if (hCacheEdit) {
        wchar_t cache_str[32];
        if (GetWindowText(hCacheEdit, cache_str, 32) > 0) {
            int cache_size = _wtoi(cache_str);
            if (cache_size > 0 && cache_size <= 1000) {
                m_config.cache_size_mb = cache_size;
            }
        }
    }
    
    EndDialog(IDOK);
}

void ConfigDialog::OnCancel(UINT uNotifyCode, int nID, HWND hWndCtl) {
    EndDialog(IDCANCEL);
}

void ConfigDialog::OnGroupingChange(UINT uNotifyCode, int nID, HWND hWndCtl) {
    // Handle grouping mode change
    // Could add preview or other real-time updates here
}

void ConfigDialog::OnSizeChange(UINT uNotifyCode, int nID, HWND hWndCtl) {
    // Handle grid size change
    // Could add preview or other real-time updates here
}

void ConfigDialog::OnThemeChange(UINT uNotifyCode, int nID, HWND hWndCtl) {
    // Handle theme change
    // Could add preview or other real-time updates here
}