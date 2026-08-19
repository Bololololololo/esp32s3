#!/bin/bash
echo "Installing customized files into 3rd party managed components"

#lv_conf.h
cp 3rd_party/lv_conf.h managed_components/lvgl__lvgl/

echo "Done"
