#!/usr/bin/env bash
set -euo pipefail

project_name='prog3_design_patterns_v2026_2'
source_code=(
  include/cafeteria.h
  include/gastos_personales.h
  include/sensores_inteligentes.h
  src/cafeteria.cpp
  src/gastos_personales.cpp
  src/sensores_inteligentes.cpp
)
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
output_path="${project_root}/${project_name}.zip"

cd "${project_root}"
rm -f "${output_path}"
zip -r -S "${output_path}" "${source_code[@]}"
echo "Archivo generado: ${output_path}"
