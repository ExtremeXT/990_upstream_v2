#!/bin/bash

abort()
{
    cd -
    echo "-----------------------------------------------"
    echo "Kernel compilation failed! Exiting..."
    echo "-----------------------------------------------"
    exit -1
}

unset_flags()
{
    cat << EOF
Usage: $(basename "$0") [options]
Options:
    -m, --model [value]    Specify the model code of the phone
    -k, --ksu [y/N]        Include KernelSU with KernelPatch Module
    -r, --recovery [y/N]   Compile kernel for an Android Recovery
    -d, --dtbs [y/N]	   Compile only DTBs
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --model|-m)
            MODEL="$2"
            shift 2
            ;;
        --ksu|-k)
            KSU_OPTION="$2"
            shift 2
            ;;
        --recovery|-r)
            RECOVERY_OPTION="$2"
            shift 2
            ;;
        --dtbs|-d)
            DTB_OPTION="$2"
            shift 2
            ;;
        *)\
            unset_flags
            exit 1
            ;;
    esac
done

echo "Preparing the build environment..."

pushd $(dirname "$0") > /dev/null
CORES=`cat /proc/cpuinfo | grep -c processor`

# Define toolchain variables
CLANG_DIR=$PWD/toolchain/clang_14
PATH=$CLANG_DIR/bin:$PATH

# Check if toolchain exists
if [ ! -f "$CLANG_DIR/bin/clang-14" ]; then
    echo "-----------------------------------------------"
    echo "Toolchain not found! Downloading..."
    echo "-----------------------------------------------"
    rm -rf $CLANG_DIR
    mkdir -p $CLANG_DIR
    pushd $CLANG_DIR > /dev/null
    curl -LJOk https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/refs/tags/android-13.0.0_r13/clang-r450784d.tar.gz
    tar xf android-13.0.0_r13-clang-r450784d.tar.gz
    rm android-13.0.0_r13-clang-r450784d.tar.gz
    echo "Cleaning up..."
    popd > /dev/null
fi

MAKE_ARGS="
LLVM=1 \
LLVM_IAS=1 \
ARCH=arm64 \
O=out
"

# Define specific variables
KERNEL_DEFCONFIG=extreme_"$MODEL"_defconfig
case $MODEL in
x1slte)
    BOARD=SRPSJ28B018KU
;;
x1s)
    BOARD=SRPSI19A018KU
;;
y2slte)
    BOARD=SRPSJ28A018KU
;;
y2s)
    BOARD=SRPSG12A018KU
;;
z3s)
    BOARD=SRPSI19B018KU
;;
c1slte)
    BOARD=SRPTC30B009KU
;;
c1s)
    BOARD=SRPTB27D009KU
;;
c2slte)
    BOARD=SRPTC30A009KU
;;
c2s)
    BOARD=SRPTB27C009KU
;;
r8s)
    BOARD=SRPTF26B014KU
;;
*)
    unset_flags
    exit
esac

if [[ "$RECOVERY_OPTION" == "y" ]]; then
    RECOVERY=recovery.config
    KSU_OPTION=n
fi

if [ -z $KSU_OPTION ]; then
    read -p "Include KernelSU with KPM (y/N): " KSU_OPTION
fi

if [[ "$KSU_OPTION" == "y" ]]; then
    KSU=ksu.config
fi

if [[ "$DTB_OPTION" == "y" ]]; then
    DTBS=y
fi

rm -rf build/out/$MODEL
mkdir -p build/out/$MODEL/zip/files
mkdir -p build/out/$MODEL/zip/META-INF/com/google/android

# Build kernel image
echo "-----------------------------------------------"
echo "Defconfig: "$KERNEL_DEFCONFIG""
if [ -z "$KSU" ]; then
    echo "KSU with KPM: N"
else
    echo "KSU with KPM: Y"
fi
if [ -z "$RECOVERY" ]; then
    echo "Recovery: N"
else
    echo "Recovery: Y"
fi

echo "-----------------------------------------------"
if [ -z "$DTBS" ]; then
    echo "Building kernel using "$MODEL.config""
else
    echo "Building DTBs using "$MODEL.config""
fi
echo "Generating configuration file..."
echo "-----------------------------------------------"
make ${MAKE_ARGS} -j$CORES exynos9830_defconfig $MODEL.config $KSU $RECOVERY || abort

if [ ! -z "$DTBS" ]; then
    MAKE_ARGS="$MAKE_ARGS dtbs"
    echo "Building DTBs"
else
    echo "Building kernel..."
fi

echo "-----------------------------------------------"
make ${MAKE_ARGS} -j$CORES || abort

# KPM Injection (always done when KSU is enabled)

if [[ "$KSU_OPTION" == "y" && -z "$DTBS" ]]; then
    echo "-----------------------------------------------"
    echo "Performing KPM Injection..."
    echo "-----------------------------------------------"
    
    mkdir -p build/out/$MODEL/SukiSUPatch
    
    # Fix: Check if Image exists and copy it correctly
    if [ -f "out/arch/arm64/boot/Image" ]; then
        cp out/arch/arm64/boot/Image build/out/$MODEL/SukiSUPatch/Image
    else
        echo "Error: Kernel Image not found at out/arch/arm64/boot/Image"
        echo "Checking for gzipped file..."
        
        # Check for gzipped image
        if [ -f "out/arch/arm64/boot/Image.gz" ]; then
            echo "Found Image.gz, extracting..."
            gunzip -c out/arch/arm64/boot/Image.gz > ~/SukiSUPatch/Image
        else
            echo "No kernel image found. KPM Injection cannot continue."
            abort
        fi
    fi
    
    cd build/out/$MODEL/SukiSUPatch/
    
    TAG=$(curl -s https://api.github.com/repos/SukiSU-Ultra/SukiSU_KernelPatch_patch/releases | \
        jq -r 'map(select(.prerelease)) | first | .tag_name')
    echo "Latest KPM patch tag is: $TAG"
    
    curl -Ls -o patch_linux "https://github.com/SukiSU-Ultra/SukiSU_KernelPatch_patch/releases/download/$TAG/patch_linux"
    chmod +x patch_linux
    
    # Make sure Image exists before proceeding
    if [ ! -f "Image" ]; then
        echo "Error: Image file not found for patching"
        cd - > /dev/null
        rm -rf build/out/$MODEL/SukiSUPatch/
        abort
    fi
    
    rm -rf out/arch/arm64/boot/Image.gz 2>/dev/null
    
    ./patch_linux
    
    # Only proceed with file operations if patch was successful
    if [ -f "oImage" ]; then
        rm -rf ./Image
        mv -f oImage Image
        gzip -k Image
        
        # Make sure the destination directory exists
        mkdir -p "$(dirname "$(readlink -f "$OLDPWD/out/arch/arm64/boot/Image.gz")")"
        
        cp build/out/$MODEL/SukiSUPatch/Image.gz "$OLDPWD/out/arch/arm64/boot/Image.gz"
        cp build/out/$MODEL/SukiSUPatch/Image "$OLDPWD/out/arch/arm64/boot/Image"
        
        echo "KPM Injection completed successfully"
    else
        echo "Error: KPM patch did not produce output file"
    fi
    
    cd - > /dev/null
    
    echo "-----------------------------------------------"
fi

## Build auxiliary boot.img files

# Define constant variables
DTB_PATH=build/out/$MODEL/dtb.img
KERNEL_PATH=build/out/$MODEL/Image
KERNEL_OFFSET=0x00008000
DTB_OFFSET=0x00000000
RAMDISK_OFFSET=0x01000000
SECOND_OFFSET=0xF0000000
TAGS_OFFSET=0x00000100
BASE=0x10000000
CMDLINE='androidboot.hardware=exynos990 loop.max_part=7'
HASHTYPE=sha1
HEADER_VERSION=2
OS_PATCH_LEVEL=2025-03
OS_VERSION=15.0.0
PAGESIZE=2048
RAMDISK=build/out/$MODEL/ramdisk.cpio.gz
OUTPUT_FILE=build/out/$MODEL/boot.img

# Copy kernel to build
if [ -z "$DTBS" ]; then
    cp out/arch/arm64/boot/Image build/out/$MODEL
fi

# Build dtb
echo "Building common exynos9830 Device Tree Blob Image..."
echo "-----------------------------------------------"
./toolchain/mkdtimg cfg_create build/out/$MODEL/dtb.img build/dtconfigs/exynos9830.cfg -d out/arch/arm64/boot/dts/exynos

# Build dtbo
echo "Building Device Tree Blob Output Image for "$MODEL"..."
echo "-----------------------------------------------"
./toolchain/mkdtimg cfg_create build/out/$MODEL/dtbo.img build/dtconfigs/$MODEL.cfg -d out/arch/arm64/boot/dts/samsung

if [ -z "$RECOVERY" ] && [ -z "$DTBS" ]; then
    # Build ramdisk
    echo "Building RAMDisk..."
    echo "-----------------------------------------------"
    pushd build/ramdisk > /dev/null
     find . ! -name . | LC_ALL=C sort | cpio -o -H newc -R root:root | gzip > ../out/$MODEL/ramdisk.cpio.gz || abort
    popd > /dev/null
    echo "-----------------------------------------------"

    # Create boot image
    echo "Creating boot image..."
    echo "-----------------------------------------------"
     ./toolchain/mkbootimg --base $BASE --board $BOARD --cmdline "$CMDLINE" --dtb $DTB_PATH \
    --dtb_offset $DTB_OFFSET --hashtype $HASHTYPE --header_version $HEADER_VERSION --kernel $KERNEL_PATH \
    --kernel_offset $KERNEL_OFFSET --os_patch_level $OS_PATCH_LEVEL --os_version $OS_VERSION --pagesize $PAGESIZE \
    --ramdisk $RAMDISK --ramdisk_offset $RAMDISK_OFFSET \
    --second_offset $SECOND_OFFSET --tags_offset $TAGS_OFFSET -o $OUTPUT_FILE || abort

    # Build zip
    echo "Building zip..."
    echo "-----------------------------------------------"
    cp build/out/$MODEL/boot.img build/out/$MODEL/zip/files/boot.img
    cp build/out/$MODEL/dtbo.img build/out/$MODEL/zip/files/dtbo.img
    cp build/update-binary build/out/$MODEL/zip/META-INF/com/google/android/update-binary
    cp build/updater-script build/out/$MODEL/zip/META-INF/com/google/android/updater-script

    version=$(grep -o 'CONFIG_LOCALVERSION="[^"]*"' arch/arm64/configs/exynos9830_defconfig | cut -d '"' -f 2)
    version=${version:1}
    pushd build/out/$MODEL/zip > /dev/null
    DATE=`date +"%d-%m-%Y_%H-%M-%S"`

    if [[ "$KSU_OPTION" == "y" ]]; then
        NAME="$version"_"$MODEL"_UNOFFICIAL_KSU_KPM_"$DATE".zip
    else
        NAME="$version"_"$MODEL"_UNOFFICIAL_"$DATE".zip
    fi
    zip -r -qq ../"$NAME" .
    popd > /dev/null
fi

popd > /dev/null
echo "Build finished successfully!"
