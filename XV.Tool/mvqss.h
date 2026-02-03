#pragma once

#include <QtCore/QString>

namespace XD
{
	const QString MODULE_DEFAUL_STYLE_ONE = "QToolButton{Color:#000000; border:1px solid #C9B0E0; border-radius:15px; background-color:#E6D6F5; background-position:center; background-repeat:repeat;}"
		"QToolButton:hover { border:2px solid #8A4FFF; }"
		"QToolButton:pressed { background-color:#8A4FFF;color:#FFFFFF; }"
		"QToolButton:disabled { color:#B399CC; }";

	const QString MODULE_SELECTED_STYLE_ONE = "QToolButton{Color:#FFFFFF; border-radius:15px; background-color:#8A4FFF; background-position:center; background-repeat:repeat;}"
		"QToolButton:disabled { color:#B399CC; }";

	const QString TITLE_STYLE_SHEET_ONE = "background-color: qlineargradient(spread:reflect, x1:0.0, y1:0.0, x2:0.0, y2:1.0, stop:0 rgba(245,233,255,255), stop:1 rgba(210,180,240,255))";

	const QString MENU_STYLE_SHEET_ONE = "background-color: qlineargradient(spread:reflect, x1:0.0, y1:0.0, x2:1.0, y2:0.0, stop:0 rgba(245,233,255,255), stop:1 rgba(210,180,240,255))";


#define XV_MSG_BOX_BUTTON_STYTLE_ONE "QPushButton {                                                                                             \
                                    border: 1px solid #7A5D8F;                                                                                  \
                                    border-radius:5px;                                                                                         \
                                    background-color: #F5E9FF;                                                                                 \
                                    width: 80px;                                                                                               \
                                    height: 35px;                                                                                              \
                                    color:#2E1437;                                                                                             \
                                 }                                                                                                             \
                                 QPushButton:hover {                                                                                           \
                                    border: 2px solid #8A4FFF;                                                                                 \
                                 }                                                                                                             \
                                 QPushButton::pressed {                                                                                        \
                                    background-color:#8A4FFF;                                                                                 \
                                    color:#FFFFFF;                                                                                            \
                                 }";

#define XV_MSG_TITLE_STYTLE_ONE "XDTitleBar{                                                                                                   \
                                background-color:#3D2D4A;                                                                                      \
                                color:#FFFFFF;                                                                                                 \
                                }                                                                                                              \
                                QWidget{                                                                                                       \
                                color: #2E1437;                                                                                               \
                                    font-family: 'Microsoft Yahei';                                                                           \
                                    font-size:13px;                                                                                           \
                                    background-color:#F5E9FF;                                                                                 \
                                }                                                                                                              \
                                QDialog{                                                                                                      \
                                color: #2E1437;                                                                                               \
                                    font-family: 'Microsoft Yahei';                                                                           \
                                    background-color:#F5E9FF;                                                                                 \
                                    font-size:13px;                                                                                           \
                                }                                                                                                              \
                                QPushButton{                                                                                                   \
                                border: 1px solid #D8C3EB;                                                                                    \
                                    border-radius:5px;                                                                                        \
                                    background-color: #E6D6F5;                                                                                \
                                    width: 60px;                                                                                              \
                                    height: 30px;                                                                                             \
                                    color:#2E1437;                                                                                            \
                                }                                                                                                             \
                                QPushButton:hover {                                                                                           \
                                    border: 1px solid #8A4FFF;                                                                                \
                                }                                                                                                             \
                                QPushButton::pressed{                                                                                         \
                                background-color:#8A4FFF;                                                                                     \
                                color:#FFFFFF;                                                                                                \
                                }                                                                                                             \
                                QPushButton#minimizeButton, #maximizeButton, #closeButton{                                                    \
                                background-color: transparent;                                                                                \
                                border: none;                                                                                                 \
                                }                                                                                                             \
                                QPushButton#minimizeButton{                                                                                   \
                                image: url(:/images/common/common_minimizeBtnBlack.png);                                                       \
                                }                                                                                                             \
                                QPushButton#minimizeButton:hover{                                                                             \
                                image: url(:/images/common/common_minimizeBtnWhite.png);                                                       \
                                }                                                                                                             \
                                QPushButton#maximizeButton{                                                                                   \
                                border: none;                                                                                                 \
                                image: url(:/images/common/common_maximizeBtnBlack.png);                                                       \
                                }                                                                                                             \
                                QPushButton#maximizeButton:hover{                                                                             \
                                image: url(:/images/common/common_maximizeBtnWhite.png);                                                       \
                                }                                                                                                             \
                                QPushButton#maximizeButton[maximizeProperty = restore]{                                                       \
                                image: url(:/images/common/common_restoreBlack.png);                                                          \
                                    padding-top: 2px;                                                                                         \
                                    padding-bottom: 2px;                                                                                      \
                                }                                                                                                             \
                                QPushButton#maximizeButton[maximizeProperty = restore]:hover{                                                 \
                                image: url(:/images/common/common_restoreWhite.png);                                                          \
                                }                                                                                                             \
                                QPushButton#maximizeButton[maximizeProperty = maximize]:hover{                                                \
                                image: url(:/images/common/common_maximizeBtnWhite.png);                                                       \
                                }                                                                                                             \
                                QPushButton#closeButton{                                                                                      \
                                border: none;                                                                                                 \
                                background-color: transparent;                                                                                \
                                image: url(:/images/common/common_closeBtnBlack_16.png);                                                      \
                                }                                                                                                             \
                                QPushButton#closeButton:hover{                                                                                \
                                image: url(:/images/common/common_closeBtnWhite_16.png);                                                      \
                                }                                                                                                             \
                                QLabel#dialogTitleLabel{                                                                                      \
                                color: #FFFFFF;                                                                                              \
                                    font-family: 'Microsoft Yahei';                                                                           \
                                }";
}