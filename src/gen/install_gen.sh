#!/bin/sh
for file in ../ui_builder.c ../ui_builder.h ../../src_builder/emul_lvgl.c ../../src_builder/emul_lvgl.h ../../src_builder/emul_lvgl_internal.h; do
  mv $file $file.bak
done

cp emul_lvgl* ../../src_builder
cp ui_builder* ..
