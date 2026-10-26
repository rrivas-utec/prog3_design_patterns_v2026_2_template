#!/usr/bin/env bash

project_name='prog3_design_patterns_v2026_2'
source_code='
  include/cafeteria.h
  include/gastos_personales.h
  include/sensores_inteligentes.h
  '
cd .. || exit 1
rm -f ${project_name}.zip
zip -r -S ${project_name} ${source_code}
