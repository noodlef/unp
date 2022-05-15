#!/bin/bash
#================================================================
#  Copyright (C) 2022 noodles. All rights reserved.
#  
#  文件名称：fsync.sh
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2022年02月15日
#  描    述：
#  
#================================================================
if [ $# -lt 1 ]; then
    echo "Usage: fsync.sh info | upload | download"
    exit 1
fi

cmd="fsync-cli --remote-ip 101.34.15.158 --remote-port 52000 --remote-dir /root/work_dir/unp --local-dir ../unp"

case $1 in
    "info")
        $cmd --cmd info;;
    "upload")
        $cmd --cmd upload;;
    "download")
        $cmd --cmd download;;
    *)
        echo "Usage: fsync.sh info | upload | download"
        exit 1;;
esac
