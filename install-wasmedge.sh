#!/usr/bin/env bash
# Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
set -euo pipefail -x

if echo "int main(void) {}" | gcc -o /dev/null -v -x c - &> /dev/stdout| grep collect | tr -s " " "\012" | grep musl; then
    # skip if the libc is musl
    exit 0
fi

curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/0.10.0/utils/install.sh | bash -s -- -e none -p ./wasmedge -v 0.10.0
