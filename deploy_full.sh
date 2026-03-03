
#!/bin/bash

# ================================================
# XVBVThickness Qt6 完整发布脚本 - 树莓派版
# 修正版：复制实际文件，不使用软链接
# ================================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 函数定义
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_result() {
    if [ $? -eq 0 ]; then
        print_success "$1"
    else
        print_error "$2"
        exit 1
    fi
}

# ================================================
# 配置参数
# ================================================
APP_NAME="XVBVThickness"
PROJECT_ROOT="/home/pi/XVBVThiness"
BUILD_DIR="${PROJECT_ROOT}/build/release"
DEPLOY_DIR="${PROJECT_ROOT}/deploy"
DEPLOY_APP_DIR="${DEPLOY_DIR}/${APP_NAME}"
DEPLOY_LIB_DIR="${DEPLOY_APP_DIR}/libs"
DEPLOY_PLUGINS_DIR="${DEPLOY_APP_DIR}/plugins"
SYS_LIB_PATH="/usr/lib/aarch64-linux-gnu"

# Qt6 路径
QT6_LIB_PATH="${SYS_LIB_PATH}"
QT6_PLUGINS_PATH="${SYS_LIB_PATH}/qt6/plugins"

# ONNX Runtime
ONNX_DIR="/home/pi/algorithm/onnxruntime-linux-aarch64-1.22.0"

# DataProc
DATAPROC_DIR="/home/pi/algorithm/DataProc-build"

# ================================================
# 开始发布
# ================================================
print_info "========================================="
print_info "开始打包 ${APP_NAME} (复制实际文件模式)"
print_info "========================================="
print_info "项目目录: ${PROJECT_ROOT}"
print_info "构建目录: ${BUILD_DIR}"
print_info "发布目录: ${DEPLOY_DIR}"

# 检查主程序
if [ ! -f "${BUILD_DIR}/${APP_NAME}" ]; then
    print_error "找不到主程序: ${BUILD_DIR}/${APP_NAME}"
    print_info "请先编译项目"
    exit 1
fi

# 清理旧的发布目录
print_info "清理旧的发布目录..."
rm -rf "${DEPLOY_APP_DIR}"
mkdir -p "${DEPLOY_APP_DIR}"
mkdir -p "${DEPLOY_LIB_DIR}"
mkdir -p "${DEPLOY_PLUGINS_DIR}/platforms"
mkdir -p "${DEPLOY_PLUGINS_DIR}/imageformats"
mkdir -p "${DEPLOY_PLUGINS_DIR}/sqldrivers"
check_result "目录创建成功" "目录创建失败"

# 复制主程序
print_info "复制主程序..."
cp "${BUILD_DIR}/${APP_NAME}" "${DEPLOY_APP_DIR}/"
print_success "主程序复制成功"

# ================================================
# 复制 Qt6 库（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制 Qt6 库 (实际文件)"
print_info "========================================="

QT_MODULES="Core Gui Widgets Concurrent Sql Xml SerialPort"
QT_ADDITIONAL="DBus Network XcbQpa OpenGL"

# 主要Qt模块
for module in $QT_MODULES $QT_ADDITIONAL; do
    # 查找实际的库文件（带版本号的）
    lib_files=$(find /usr -name "libQt6${module}.so.*" 2>/dev/null | grep -v "nsight-compute" | sort -V | tail -1)
    
    if [ -n "$lib_files" ]; then
        for lib_file in $lib_files; do
            if [ -f "$lib_file" ] && [ ! -L "$lib_file" ]; then
                cp -v "$lib_file" "${DEPLOY_LIB_DIR}/"
                print_info "复制: $(basename "$lib_file")"
            fi
        done
    else
        print_warning "找不到 Qt6 ${module} 的实际库文件"
    fi
done

# 复制 Qt6 核心库
QT_CORE_LIBS="
libQt6Core.so.*
libQt6Gui.so.*
libQt6Widgets.so.*
libQt6Sql.so.*
libQt6Xml.so.*
libQt6SerialPort.so.*
libQt6Concurrent.so.*
libQt6DBus.so.*
libQt6Network.so.*
libQt6OpenGL.so.*
libQt6XcbQpa.so.*
libQt6EglFSDeviceIntegration.so.*
libQt6EglFsKmsSupport.so.*
libQt6KmsSupport.so.*
libQt6DeviceDiscoverySupport.so.*
libQt6FbSupport.so.*
libQt6ServiceSupport.so.*
libQt6ThemeSupport.so.*
"

for lib_pattern in $QT_CORE_LIBS; do
    lib_files=$(find "${SYS_LIB_PATH}" -name "${lib_pattern}" 2>/dev/null | grep -v "nsight-compute" | sort -V | tail -1)
    if [ -n "$lib_files" ]; then
        for lib_file in $lib_files; do
            if [ -f "$lib_file" ] && [ ! -L "$lib_file" ]; then
                cp -v "$lib_file" "${DEPLOY_LIB_DIR}/"
            fi
        done
    fi
done

# ================================================
# 复制 Qt6 插件（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制 Qt6 插件 (实际文件)"
print_info "========================================="

# 查找插件目录
PLUGIN_DIR=$(find /usr -type d -path "*/qt6/plugins" 2>/dev/null | head -1)
if [ -d "$PLUGIN_DIR" ]; then
    print_info "找到插件目录: $PLUGIN_DIR"
    
    # 复制 platforms 插件
    if [ -d "${PLUGIN_DIR}/platforms" ]; then
        for plugin in "${PLUGIN_DIR}/platforms"/*.so; do
            if [ -f "$plugin" ] && [ ! -L "$plugin" ]; then
                cp -v "$plugin" "${DEPLOY_PLUGINS_DIR}/platforms/"
            fi
        done
        print_info "复制 platforms 插件完成"
    fi
    
    # 复制 imageformats 插件
    if [ -d "${PLUGIN_DIR}/imageformats" ]; then
        for plugin in "${PLUGIN_DIR}/imageformats"/*.so; do
            if [ -f "$plugin" ] && [ ! -L "$plugin" ]; then
                cp -v "$plugin" "${DEPLOY_PLUGINS_DIR}/imageformats/"
            fi
        done
        print_info "复制 imageformats 插件完成"
    fi
    
    # 复制 sqldrivers 插件
    if [ -d "${PLUGIN_DIR}/sqldrivers" ]; then
        mkdir -p "${DEPLOY_PLUGINS_DIR}/sqldrivers"
        for plugin in "${PLUGIN_DIR}/sqldrivers"/*.so; do
            if [ -f "$plugin" ] && [ ! -L "$plugin" ]; then
                cp -v "$plugin" "${DEPLOY_PLUGINS_DIR}/sqldrivers/"
            fi
        done
        print_info "复制 sqldrivers 插件完成"
    fi
    
    # 复制其他可能的插件
    for plugin_type in generic iconengines styles; do
        if [ -d "${PLUGIN_DIR}/${plugin_type}" ]; then
            mkdir -p "${DEPLOY_PLUGINS_DIR}/${plugin_type}"
            for plugin in "${PLUGIN_DIR}/${plugin_type}"/*.so; do
                if [ -f "$plugin" ] && [ ! -L "$plugin" ]; then
                    cp -v "$plugin" "${DEPLOY_PLUGINS_DIR}/${plugin_type}/"
                fi
            done
        fi
    done
else
    print_warning "找不到 Qt6 插件目录"
fi

# ================================================
# 复制 GDCM 库（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制 GDCM 库 (实际文件)"
print_info "========================================="

# 查找所有 GDCM 实际库文件
gdcm_files=$(find "${SYS_LIB_PATH}" -name "libgdcm*.so.*" 2>/dev/null | grep -v "\.so$" | sort -u)

if [ -n "$gdcm_files" ]; then
    for lib_file in $gdcm_files; do
        if [ -f "$lib_file" ] && [ ! -L "$lib_file" ]; then
            cp -v "$lib_file" "${DEPLOY_LIB_DIR}/"
        fi
    done
    print_info "GDCM 库复制完成"
else
    print_warning "找不到 GDCM 库"
fi

# ================================================
# 复制 OpenCV 库（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制 OpenCV 库 (实际文件)"
print_info "========================================="

# 查找所有 OpenCV 实际库文件
opencv_files=$(find "${SYS_LIB_PATH}" -name "libopencv_*.so.*" 2>/dev/null | grep -v "\.so$" | sort -u)

if [ -n "$opencv_files" ]; then
    for lib_file in $opencv_files; do
        if [ -f "$lib_file" ] && [ ! -L "$lib_file" ]; then
            cp -v "$lib_file" "${DEPLOY_LIB_DIR}/"
        fi
    done
    print_info "OpenCV 库复制完成"
else
    print_warning "找不到 OpenCV 库"
fi

# ================================================
# 复制 ONNX Runtime（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制 ONNX Runtime (实际文件)"
print_info "========================================="

if [ -d "$ONNX_DIR" ]; then
    # 复制实际库文件（不是软链接）
    onnx_files=$(find "${ONNX_DIR}/lib" -name "*.so.*" 2>/dev/null | grep -v "\.so$" | sort -u)
    if [ -n "$onnx_files" ]; then
        for lib_file in $onnx_files; do
            if [ -f "$lib_file" ] && [ ! -L "$lib_file" ]; then
                cp -v "$lib_file" "${DEPLOY_LIB_DIR}/"
            fi
        done
    fi
    print_info "ONNX Runtime 库复制完成"
else
    print_warning "找不到 ONNX Runtime 目录"
fi

# ================================================
# 复制 DataProc（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制 DataProc (实际文件)"
print_info "========================================="

if [ -f "${DATAPROC_DIR}/libDataProc.so" ]; then
    # 查找实际文件
    dataproc_file=$(find "${DATAPROC_DIR}" -name "libDataProc.so.*" 2>/dev/null | grep -v "\.so$" | head -1)
    if [ -n "$dataproc_file" ] && [ -f "$dataproc_file" ]; then
        cp -v "$dataproc_file" "${DEPLOY_LIB_DIR}/"
        print_info "DataProc 库复制完成"
    else
        print_warning "找不到 DataProc 实际库文件"
    fi
else
    print_warning "找不到 DataProc 库"
fi

# ================================================
# 复制系统库（复制实际文件）
# ================================================
print_info "========================================="
print_info "复制系统库 (实际文件)"
print_info "========================================="

# 系统库模式列表
SYS_LIB_PATTERNS="
libz.so.*
libminizip.so.*
libcrypto.so.*
libssl.so.*
libsqlite3.so.*
libpng16.so.*
libjpeg.so.*
libtiff.so.*
libwebp.so.*
libfreetype.so.*
libfontconfig.so.*
libharfbuzz.so.*
libpcre2-16.so.*
libdouble-conversion.so.*
libicudata.so.*
libicui18n.so.*
libicuuc.so.*
libglib-2.0.so.*
libgthread-2.0.so.*
libEGL.so.*
libGLESv2.so.*
libgbm.so.*
libdrm.so.*
libinput.so.*
libxkbcommon.so.*
libX11.so.*
libXext.so.*
libxcb.so.*
libxcb-xinerama.so.*
libxcb-xinput.so.*
libxcb-xfixes.so.*
libxcb-shape.so.*
libxcb-randr.so.*
libxcb-image.so.*
libxcb-icccm.so.*
libxcb-sync.so.*
libxcb-xkb.so.*
libxcb-render.so.*
libxcb-shm.so.*
libxcb-util.so.*
"

for pattern in $SYS_LIB_PATTERNS; do
    lib_files=$(find "${SYS_LIB_PATH}" -name "${pattern}" 2>/dev/null | grep -v "\.so$" | sort -V | tail -1)
    if [ -n "$lib_files" ]; then
        for lib_file in $lib_files; do
            if [ -f "$lib_file" ] && [ ! -L "$lib_file" ]; then
                cp -v "$lib_file" "${DEPLOY_LIB_DIR}/" 2>/dev/null || true
            fi
        done
    fi
done

# ================================================
# 使用 ldd 收集其他依赖（复制实际文件）
# ================================================
print_info "========================================="
print_info "收集其他依赖 (实际文件)"
print_info "========================================="

TEMP_LIBS=$(mktemp)
ldd "${BUILD_DIR}/${APP_NAME}" 2>/dev/null | grep "=> /" | awk '{print $3}' | sort -u > "$TEMP_LIBS"

while read lib; do
    if [ -f "$lib" ] && [ ! -L "$lib" ]; then
        lib_name=$(basename "$lib")
        if [ ! -f "${DEPLOY_LIB_DIR}/${lib_name}" ]; then
            cp -v "$lib" "${DEPLOY_LIB_DIR}/"
        fi
    else
        # 如果是软链接，找到实际文件
        real_lib=$(readlink -f "$lib")
        if [ -f "$real_lib" ]; then
            lib_name=$(basename "$real_lib")
            if [ ! -f "${DEPLOY_LIB_DIR}/${lib_name}" ]; then
                cp -v "$real_lib" "${DEPLOY_LIB_DIR}/"
            fi
        fi
    fi
done < "$TEMP_LIBS"

rm -f "$TEMP_LIBS"

# ================================================
# 创建运行时软链接（在打包目录内创建，方便程序查找）
# ================================================
print_info "========================================="
print_info "创建运行时软链接"
print_info "========================================="

cd "${DEPLOY_LIB_DIR}"

# 为所有库创建 .so 链接
for lib in *.so.*.*.*; do
    if [ -f "$lib" ]; then
        # 提取基础名称
        base_name=$(echo "$lib" | sed 's/\.so\.[0-9.]\+$/.so/')
        if [ ! -L "$base_name" ] && [ ! -f "$base_name" ]; then
            ln -sf "$lib" "$base_name"
            print_info "创建链接: $base_name -> $lib"
        fi
        
        # 提取主版本号链接 (例如 .so.3)
        if [[ "$lib" =~ \.so\.([0-9]+)\. ]]; then
            major_ver="${BASH_REMATCH[1]}"
            major_link=$(echo "$lib" | sed "s/\.so\.[0-9.]\+$/.so.${major_ver}/")
            if [ ! -L "$major_link" ] && [ ! -f "$major_link" ]; then
                ln -sf "$lib" "$major_link"
                print_info "创建链接: $major_link -> $lib"
            fi
        fi
    fi
done

# 特殊处理 GDCM 的版本链接
for lib in libgdcm*.so.*.*.*; do
    if [ -f "$lib" ]; then
        # 创建 .so.3.0 链接
        if [[ "$lib" =~ lib(gdcm[^.]+)\.so\.([0-9.]+)$ ]]; then
            base_name="lib${BASH_REMATCH[1]}.so"
            version="${BASH_REMATCH[2]}"
            major_minor=$(echo "$version" | cut -d. -f1,2)
            
            if [ ! -L "${base_name}.${major_minor}" ] && [ ! -f "${base_name}.${major_minor}" ]; then
                ln -sf "$lib" "${base_name}.${major_minor}"
                print_info "创建链接: ${base_name}.${major_minor} -> $lib"
            fi
        fi
    fi
done

# ================================================
# 创建启动脚本
# ================================================
print_info "========================================="
print_info "创建启动脚本"
print_info "========================================="

cat > "${DEPLOY_APP_DIR}/run.sh" << 'EOF'
#!/bin/bash

# XVBVThickness 启动脚本
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# 设置库路径
export LD_LIBRARY_PATH="${SCRIPT_DIR}/libs:${LD_LIBRARY_PATH}"

# 设置Qt插件路径
export QT_PLUGIN_PATH="${SCRIPT_DIR}/plugins"

# Qt平台配置 - 根据目标系统调整
# export QT_QPA_PLATFORM=eglfs  # 如果使用EGLFS
# export QT_QPA_PLATFORM=linuxfb # 如果使用LinuxFB
export QT_QPA_PLATFORM=xcb      # 如果使用X11

# 调试信息（可选）
echo "=========================================="
echo "XVBVThickness 启动"
echo "工作目录: ${SCRIPT_DIR}"
echo "库路径: ${LD_LIBRARY_PATH}"
echo "插件路径: ${QT_PLUGIN_PATH}"
echo "=========================================="

# 进入程序目录
cd "${SCRIPT_DIR}"

# 运行程序
exec ./XVBVThickness "$@"
EOF

chmod +x "${DEPLOY_APP_DIR}/run.sh"
print_success "启动脚本创建成功"

# ================================================
# 创建测试脚本
# ================================================
print_info "创建测试脚本..."

cat > "${DEPLOY_APP_DIR}/test.sh" << 'EOF'
#!/bin/bash

# 测试脚本
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "=========================================="
echo "XVBVThickness 测试脚本"
echo "=========================================="

# 检查库文件
echo "检查库文件:"
find "${SCRIPT_DIR}/libs" -name "*.so*" | wc -l
echo ""

# 检查主程序
if [ -f "${SCRIPT_DIR}/XVBVThickness" ]; then
    echo "✓ 主程序存在"
    ls -l "${SCRIPT_DIR}/XVBVThickness"
else
    echo "✗ 主程序不存在"
fi
echo ""

# 检查Qt插件
echo "Qt插件检查:"
ls -la "${SCRIPT_DIR}/plugins/platforms/" 2>/dev/null | head -5
echo ""

# 测试库依赖
echo "测试库依赖:"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/libs:${LD_LIBRARY_PATH}"
ldd "${SCRIPT_DIR}/XVBVThickness" 2>/dev/null | grep "not found"
if [ $? -eq 0 ]; then
    echo "✗ 有缺失的依赖库"
else
    echo "✓ 所有依赖库都找到"
fi
echo "=========================================="
EOF

chmod +x "${DEPLOY_APP_DIR}/test.sh"
print_success "测试脚本创建成功"

# ================================================
# 创建README文件
# ================================================
print_info "创建README文件..."

cat > "${DEPLOY_APP_DIR}/README.txt" << EOF
==========================================
XVBVThickness 电缆厚度测量系统
打包日期: $(date +%Y-%m-%d)
==========================================

目录结构:
- XVBVThickness        - 主程序
- libs/                - 所有依赖库（实际文件）
- plugins/             - Qt插件
  - platforms/         - 平台插件
  - imageformats/      - 图像格式插件
  - sqldrivers/        - 数据库驱动
- run.sh               - 启动脚本
- test.sh              - 测试脚本

使用方法:
1. 直接运行: ./run.sh
2. 测试环境: ./test.sh

环境要求:
- 操作系统: Linux (ARM64/aarch64)
- 依赖: 已包含所有库文件

注意事项:
- 所有库文件都是实际文件，不是软链接
- 如果目标系统缺少某些系统库，可能需要额外安装
- 启动脚本会自动设置库路径

==========================================
EOF

# ================================================
# 创建压缩包
# ================================================
print_info "========================================="
print_info "创建压缩包"
print_info "========================================="

cd "${DEPLOY_DIR}"
PACKAGE_NAME="${APP_NAME}-full-$(date +%Y%m%d)-actual-files.tar.gz"
tar -czf "${PACKAGE_NAME}" "${APP_NAME}"
check_result "创建压缩包成功" "创建压缩包失败"

# ================================================
# 显示统计信息
# ================================================
print_info "========================================="
print_info "发布统计"
print_info "========================================="

TOTAL_SIZE=$(du -sh "${DEPLOY_APP_DIR}" | cut -f1)
LIB_COUNT=$(find "${DEPLOY_LIB_DIR}" -name "*.so*" -type f | wc -l)
PLUGIN_COUNT=$(find "${DEPLOY_PLUGINS_DIR}" -name "*.so" -type f | wc -l)

print_info "实际库文件: ${LIB_COUNT} 个"
print_info "插件文件: ${PLUGIN_COUNT} 个"
print_info "总大小: ${TOTAL_SIZE}"
print_info "发布目录: ${DEPLOY_APP_DIR}"
print_info "压缩包: ${DEPLOY_DIR}/${PACKAGE_NAME}"
print_info ""
print_info "压缩包大小: $(du -h "${DEPLOY_DIR}/${PACKAGE_NAME}" | cut -f1)"
print_success "========================================="
print_success "打包完成！所有文件都是实际文件，可以直接发布"
print_success "========================================="

# 提示下一步
echo ""
echo -e "${YELLOW}下一步操作:${NC}"
echo "1. 测试本地打包: ${DEPLOY_APP_DIR}/test.sh"
echo "2. 传输到目标机器: scp ${DEPLOY_DIR}/${PACKAGE_NAME} pi@192.168.2.158:/home/pi/XVBVThicknessNew/"
echo "3. 在目标机器上解压: tar -xzf ${PACKAGE_NAME}"
echo "4. 运行测试: ./test.sh"
echo "5. 启动程序: ./run.sh"
