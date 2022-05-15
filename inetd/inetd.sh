#!/bin/bash
#================================================================
#  Copyright (C) 2021 noodles. All rights reserved.
#  
#  文件名称：inetd.sh
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2021年12月19日
#  描    述：
#  
#================================================================
pid=

function get_pid()
{
    pid=`ps -ef | grep inetd-server | grep -v grep | awk '{print $2}'`
    [ ! $pid ] && echo "Inetd-server is NOT found!!!" && exit 1
}

if [ $# -lt 1 ]; then
    echo "Usage: inetd.sh start | stop | reload | status"
    exit 1
fi

case $1 in
    "start")
        inetd-server --config-file /etc/inetd/inetd-conf.ini;;
    "restart")
        pid=`ps -ef | grep inetd-server | grep -v grep | awk '{print $2}'`
        [ $pid ] && kill -s 9 $pid
        inetd-server --config-file /etc/inetd/inetd-conf.ini;;
    "stop")
        get_pid
        kill -s 9 $pid;;
    "reload")
        get_pid
        kill -s SIGHUP $pid;;
    "status")
        get_pid
        ps -eo pid,stat,wchan:14 | head -n 1
        ps -eo pid,stat,wchan:14 | grep $pid;;
    *)
        echo "Usage: inetd.sh start | stop | reload | status"
        exit 1;;
esac

[ $? -eq 0 ] || echo "$1 inetd-server failled, exit-code: $?"
