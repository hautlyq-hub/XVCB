
#!/bin/bash

# ====================================================
# XVBVThickness 完整打包发布脚本 (树莓派版)
# 功能：打包应用程序及其所有依赖库
# ====================================================

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置参数
APP_NAME="XVBVThickness"
PROJECT_DIR=$(pwd)
BUILD_DIR="${PROJECT_DIR}/build/release"
DEPLOY_DIR="${PROJECT_DIR}/deploy"
PACKAGE_DIR="${DEPLOY_DIR}/${APP_NAME}"
VERSION="1.0.0"
DATE=$(date +%Y%m%d)
PACKAGE_NAME="${APP_NAME}_${VERSION}_${DATE}"

# 输出信息
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}开始打包 ${APP_NAME}${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "项目目录: ${PROJECT_DIR}"
echo -e "构建目录: ${BUILD_DIR}"
echo -e "打包目录: ${PACKAGE_DIR}"
echo -e "版本: ${VERSION}"
echo -e "${BLUE}========================================${NC}"

# 检查是否以root运行
if [ "$EUID" -eq 0 ]; then 
    echo -e "${RED}警告: 不建议以root用户运行此脚本${NC}"
    read -p "是否继续? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 清理旧的打包目录
echo -e "${YELLOW}清理旧的打包目录...${NC}"
rm -rf "${DEPLOY_DIR}"
mkdir -p "${PACKAGE_DIR}"

# ==================== 1. 编译应用程序 ====================
echo -e "${GREEN}[1/8] 编译应用程序...${NC}"

# 清理并重新构建
make clean || true
qmake6 CONFIG+=release
make -j$(nproc)

if [ ! -f "${BUILD_DIR}/${APP_NAME}" ]; then
    echo -e "${RED}错误: 编译失败，找不到可执行文件${NC}"
    exit 1
fi

# ==================== 2. 创建目录结构 ====================
echo -e "${GREEN}[2/8] 创建目录结构...${NC}"
mkdir -p "${PACKAGE_DIR}/bin"
mkdir -p "${PACKAGE_DIR}/lib"
mkdir -p "${PACKAGE_DIR}/plugins"
mkdir -p "${PACKAGE_DIR}/platforms"
mkdir -p "${PACKAGE_DIR}/styles"
mkdir -p "${PACKAGE_DIR}/imageformats"
mkdir -p "${PACKAGE_DIR}/iconengines"
mkdir -p "${PACKAGE_DIR}/translations"
mkdir -p "${PACKAGE_DIR}/config"
mkdir -p "${PACKAGE_DIR}/logs"
mkdir -p "${PACKAGE_DIR}/data"
mkdir -p "${PACKAGE_DIR}/algorithms/models"
mkdir -p "${PACKAGE_DIR}/scripts"
mkdir -p "${PACKAGE_DIR}/doc"

# ==================== 3. 复制应用程序文件 ====================
echo -e "${GREEN}[3/8] 复制应用程序文件...${NC}"
cp -v "${BUILD_DIR}/${APP_NAME}" "${PACKAGE_DIR}/bin/"

# ==================== 4. 复制依赖库 ====================
echo -e "${GREEN}[4/8] 复制依赖库...${NC}"

# 获取可执行文件
EXECUTABLE="${PACKAGE_DIR}/bin/${APP_NAME}"

# 使用ldd查找所有依赖库
echo "查找依赖库..."
LIBS=$(ldd "${EXECUTABLE}" | grep "=> /" | awk '{print $3}' | sort -u)

# Qt库路径
QT_LIB_PATH=$(qmake6 -query QT_INSTALL_LIBS)
QT_PLUGIN_PATH=$(qmake6 -query QT_INSTALL_PLUGINS)
QT_QML_PATH=$(qmake6 -query QT_INSTALL_QML)

# 复制依赖库
for lib in $LIBS; do
    if [ -f "$lib" ]; then
        # 检查是否是系统库（跳过系统关键库）
        if [[ "$lib" != "/lib/"* && "$lib" != "/usr/lib/"*"/ld-linux"* ]]; then
            echo "复制库: $lib"
            cp -v "$lib" "${PACKAGE_DIR}/lib/" 2>/dev/null || true
        else
            echo "跳过系统库: $lib"
        fi
    fi
done

# 复制Qt库
echo "复制Qt库..."
QT_LIBS=(
    "libQt6Core.so*"
    "libQt6Gui.so*"
    "libQt6Widgets.so*"
    "libQt6Sql.so*"
    "libQt6Xml.so*"
    "libQt6SerialPort.so*"
    "libQt6Concurrent.so*"
    "libQt6DBus.so*"
    "libQt6Network.so*"
    "libQt6OpenGL.so*"
    "libQt6XcbQpa.so*"
)

for qtlib in "${QT_LIBS[@]}"; do
    find "${QT_LIB_PATH}" -name "${qtlib}" -exec cp -v {} "${PACKAGE_DIR}/lib/" \;
done

# 复制OpenCV库
echo "复制OpenCV库..."
OPENCV_LIBS=$(pkg-config --libs-only-L opencv4 | sed 's/-L//g')
if [ -n "$OPENCV_LIBS" ]; then
    for libpath in $OPENCV_LIBS; do
        find "${libpath}" -name "libopencv_*.so*" -exec cp -v {} "${PACKAGE_DIR}/lib/" \;
    done
fi

# 复制ONNX Runtime库
echo "复制ONNX Runtime库..."
ONNX_DIR="/home/pi/algorithm/onnxruntime-linux-aarch64-1.22.0"
if [ -d "$ONNX_DIR" ]; then
    cp -v "${ONNX_DIR}/lib/libonnxruntime.so"* "${PACKAGE_DIR}/lib/" 2>/dev/null || true
fi

# 复制DataProc库
echo "复制DataProc库..."
DATAPROC_DIR="/home/pi/algorithm/DataProc-build"
if [ -d "$DATAPROC_DIR" ]; then
    cp -v "${DATAPROC_DIR}/libDataProc.so"* "${PACKAGE_DIR}/lib/" 2>/dev/null || true
fi

# 复制minizip库
echo "复制minizip库..."
find /usr/lib -name "libminizip.so*" -exec cp -v {} "${PACKAGE_DIR}/lib/" \; 2>/dev/null || true

# ==================== 5. 复制Qt插件 ====================
echo -e "${GREEN}[5/8] 复制Qt插件...${NC}"

# 复制platform插件
cp -rv "${QT_PLUGIN_PATH}/platforms/libqlinuxfb.so" "${PACKAGE_DIR}/platforms/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/platforms/libqeglfs.so" "${PACKAGE_DIR}/platforms/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/platforms/libqminimal.so" "${PACKAGE_DIR}/platforms/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/platforms/libqoffscreen.so" "${PACKAGE_DIR}/platforms/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/platforms/libqvnc.so" "${PACKAGE_DIR}/platforms/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/platforms/libqxcb.so" "${PACKAGE_DIR}/platforms/" 2>/dev/null || true

# 复制styles插件
cp -rv "${QT_PLUGIN_PATH}/styles/libqgtk3style.so" "${PACKAGE_DIR}/styles/" 2>/dev/null || true

# 复制imageformats插件
cp -rv "${QT_PLUGIN_PATH}/imageformats" "${PACKAGE_DIR}/" 2>/dev/null || true

# 复制iconengines插件
cp -rv "${QT_PLUGIN_PATH}/iconengines" "${PACKAGE_DIR}/" 2>/dev/null || true

# 复制sqldrivers插件
mkdir -p "${PACKAGE_DIR}/sqldrivers"
cp -rv "${QT_PLUGIN_PATH}/sqldrivers/libqsqlite.so" "${PACKAGE_DIR}/sqldrivers/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/sqldrivers/libqsqlmysql.so" "${PACKAGE_DIR}/sqldrivers/" 2>/dev/null || true

# 复制其他可能需要的插件
cp -rv "${QT_PLUGIN_PATH}/generic" "${PACKAGE_DIR}/" 2>/dev/null || true
cp -rv "${QT_PLUGIN_PATH}/xcbglintegrations" "${PACKAGE_DIR}/" 2>/dev/null || true

# ==================== 6. 复制配置文件和数据文件 ====================
echo -e "${GREEN}[6/8] 复制配置文件和数据文件...${NC}"

# 复制翻译文件
cp -v *.ts "${PACKAGE_DIR}/translations/" 2>/dev/null || true
cp -v *.qm "${PACKAGE_DIR}/translations/" 2>/dev/null || true
cp -rv "${QT_LIB_PATH}/../../translations"/* "${PACKAGE_DIR}/translations/" 2>/dev/null || true

# 复制资源文件
if [ -f "${PROJECT_DIR}/mvResources.qrc" ]; then
    cp -v "${PROJECT_DIR}/mvResources.qrc" "${PACKAGE_DIR}/"
fi

# 复制默认配置文件
if [ -f "${PROJECT_DIR}/config.ini" ]; then
    cp -v "${PROJECT_DIR}/config.ini" "${PACKAGE_DIR}/config/"
fi

# 复制算法模型文件
if [ -d "/home/pi/algorithm/models" ]; then
    cp -rv "/home/pi/algorithm/models"/* "${PACKAGE_DIR}/algorithms/models/" 2>/dev/null || true
fi

# ==================== 7. 创建启动脚本 ====================
echo -e "${GREEN}[7/8] 创建启动脚本...${NC}"

# 创建启动脚本
cat > "${PACKAGE_DIR}/run.sh" << 'EOF'
#!/bin/bash

# XVBVThickness 启动脚本

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${SCRIPT_DIR}"

# 设置环境变量
export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${SCRIPT_DIR}"
export QT_QPA_PLATFORM_PLUGIN_PATH="${SCRIPT_DIR}/platforms"
export QT_LOGGING_RULES="*.debug=false"
export QT_QPA_FB_HIDECURSOR=0  # 是否隐藏鼠标光标

# 如果使用EGLFS，可以设置以下变量
# export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
# export QT_QPA_EGLFS_KMS_ATOMIC=1

# 创建日志目录
mkdir -p "${SCRIPT_DIR}/logs"

# 日志文件
LOG_FILE="${SCRIPT_DIR}/logs/application_$(date +%Y%m%d_%H%M%S).log"

# 运行应用程序
echo "========================================"
echo "XVBVThickness 启动中..."
echo "工作目录: ${SCRIPT_DIR}"
echo "日志文件: ${LOG_FILE}"
echo "========================================"

# 检查配置文件
if [ ! -f "${SCRIPT_DIR}/config/config.ini" ]; then
    echo "警告: 配置文件不存在，将使用默认配置"
fi

# 启动应用
./bin/XVBVThickness "$@" 2>&1 | tee -a "${LOG_FILE}"

# 退出码
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then
    echo "应用程序异常退出，退出码: $EXIT_CODE"
    echo "请查看日志文件了解详情: ${LOG_FILE}"
fi

exit $EXIT_CODE
EOF

chmod +x "${PACKAGE_DIR}/run.sh"

# 创建安装脚本
cat > "${PACKAGE_DIR}/install.sh" << 'EOF'
#!/bin/bash

# XVBVThickness 安装脚本

set -e

echo "========================================"
echo "XVBVThickness 安装程序"
echo "========================================"

# 检查是否以root运行
if [ "$EUID" -ne 0 ]; then 
    echo "请以root权限运行此脚本"
    exit 1
fi

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INSTALL_DIR="/opt/XVBVThickness"
DESKTOP_FILE="/usr/share/applications/XVBVThickness.desktop"

echo "安装到: ${INSTALL_DIR}"

# 创建安装目录
mkdir -p "${INSTALL_DIR}"

# 复制文件
echo "复制文件..."
cp -rv "${SCRIPT_DIR}"/* "${INSTALL_DIR}/"

# 创建桌面快捷方式
echo "创建桌面快捷方式..."
cat > "${DESKTOP_FILE}" << 'INNEREOF'
[Desktop Entry]
Name=XVBVThickness
Comment=X-Ray Cable Thickness Measurement System
Exec=/opt/XVBVThickness/run.sh
Icon=/opt/XVBVThickness/icon.png
Terminal=false
Type=Application
Categories=Science;Engineering;
StartupNotify=true
INNEREOF

# 创建符号链接
ln -sf "${INSTALL_DIR}/run.sh" /usr/local/bin/XVBVThickness

# 设置权限
chmod +x "${INSTALL_DIR}/run.sh"
chmod +x "${INSTALL_DIR}/bin/XVBVThickness"
chmod -R 755 "${INSTALL_DIR}"

echo "========================================"
echo "安装完成！"
echo "您可以通过以下方式启动："
echo "1. 命令行: XVBVThickness"
echo "2. 桌面菜单: 应用程序菜单中找到 XVBVThickness"
echo "========================================"
EOF

chmod +x "${PACKAGE_DIR}/install.sh"

# 创建卸载脚本
cat > "${PACKAGE_DIR}/uninstall.sh" << 'EOF'
#!/bin/bash

# XVBVThickness 卸载脚本

set -e

echo "========================================"
echo "XVBVThickness 卸载程序"
echo "========================================"

# 检查是否以root运行
if [ "$EUID" -ne 0 ]; then 
    echo "请以root权限运行此脚本"
    exit 1
fi

read -p "确定要卸载 XVBVThickness 吗? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
fi

echo "卸载中..."

# 删除安装目录
rm -rf /opt/XVBVThickness

# 删除桌面文件
rm -f /usr/share/applications/XVBVThickness.desktop

# 删除符号链接
rm -f /usr/local/bin/XVBVThickness

echo "卸载完成！"
EOF

chmod +x "${PACKAGE_DIR}/uninstall.sh"

# 创建README文件
cat > "${PACKAGE_DIR}/README.txt" << EOF
========================================
XVBVThickness 电缆厚度测量系统
版本: ${VERSION}
发布日期: ${DATE}
========================================

目录结构:
- bin/: 可执行文件
- lib/: 共享库文件
- platforms/: Qt平台插件
- imageformats/: Qt图像格式插件
- iconengines/: Qt图标引擎插件
- sqldrivers/: Qt数据库驱动
- config/: 配置文件
- logs/: 日志文件
- data/: 数据文件
- algorithms/models/: 算法模型文件
- scripts/: 辅助脚本
- doc/: 文档

运行方法:
1. 直接运行: ./run.sh
2. 安装到系统: sudo ./install.sh
3. 卸载: sudo ./uninstall.sh

配置文件:
- config/config.ini: 主配置文件

日志文件:
- logs/application_*.log: 应用程序日志

系统要求:
- 操作系统: Raspberry Pi OS (64位)
- 硬件: 树莓派4B或更高版本
- 内存: 至少4GB
- 存储: 至少1GB可用空间

联系方式:
- 技术支持: support@example.com
- 公司网站: http://www.example.com

版权信息:
Copyright (c) 2024 保留所有权利
========================================
EOF

# ==================== 8. 打包压缩 ====================
echo -e "${GREEN}[8/8] 打包压缩...${NC}"

cd "${DEPLOY_DIR}"

# 创建tar.gz包
tar -czf "${PACKAGE_NAME}.tar.gz" "${APP_NAME}"

# 创建zip包
zip -r "${PACKAGE_NAME}.zip" "${APP_NAME}" > /dev/null

# 创建Debian包（可选）
echo -e "${YELLOW}是否创建Debian安装包? (y/n)${NC}"
read -n 1 -r create_deb
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    # 创建DEB包结构
    DEB_DIR="${DEPLOY_DIR}/deb_build"
    mkdir -p "${DEB_DIR}/DEBIAN"
    mkdir -p "${DEB_DIR}/opt/${APP_NAME}"
    mkdir -p "${DEB_DIR}/usr/share/applications"
    mkdir -p "${DEB_DIR}/usr/local/bin"
    
    # 复制文件到DEB目录
    cp -r "${PACKAGE_DIR}"/* "${DEB_DIR}/opt/${APP_NAME}/"
    
    # 创建control文件
    cat > "${DEB_DIR}/DEBIAN/control" << EOF
Package: ${APP_NAME}
Version: ${VERSION}
Section: science
Priority: optional
Architecture: arm64
Depends: libc6 (>= 2.28), libstdc++6 (>= 8), libgcc1 (>= 1:8)
Maintainer: Your Name <your.email@example.com>
Description: X-Ray Cable Thickness Measurement System
 A professional system for measuring cable thickness using X-ray technology.
 Supports real-time image acquisition, processing, and analysis.
EOF
    
    # 创建桌面文件
    cat > "${DEB_DIR}/usr/share/applications/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Name=${APP_NAME}
Comment=X-Ray Cable Thickness Measurement System
Exec=/opt/${APP_NAME}/run.sh
Icon=/opt/${APP_NAME}/icon.png
Terminal=false
Type=Application
Categories=Science;Engineering;
EOF
    
    # 创建符号链接
    ln -sf "/opt/${APP_NAME}/run.sh" "${DEB_DIR}/usr/local/bin/${APP_NAME}"
    
    # 构建DEB包
    dpkg-deb --build "${DEB_DIR}" "${DEPLOY_DIR}/${PACKAGE_NAME}.deb"
    
    # 清理
    rm -rf "${DEB_DIR}"
    
    echo -e "${GREEN}Debian包创建完成: ${PACKAGE_NAME}.deb${NC}"
fi

# ==================== 完成 ====================
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}打包完成！${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "打包文件位于: ${DEPLOY_DIR}"
echo -e ""
echo -e "生成的文件:"
ls -lh "${DEPLOY_DIR}/" | grep -E "(${PACKAGE_NAME}|${APP_NAME})"
echo -e ""
echo -e "打包内容:"
du -sh "${PACKAGE_DIR}"
echo -e "${BLUE}========================================${NC}"
