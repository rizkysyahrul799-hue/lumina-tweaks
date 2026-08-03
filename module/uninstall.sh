#
# Copyright (C) 2024-2026 Rem01Gaming
# Copyright (C) 2026 LUMina Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


killall -9 luminad lumina encored 2>/dev/null

rm -rf /data/adb/.config/lumina
rm -f /data/adb/service.d/.lumina_cleanup.sh
rm -f /data/adb/service.d/.encore_cleanup.sh

need_gone="luminad lumina_profiler lumina_utility lumina_log encored encore_profiler encore_utility encore_log"
manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"

for dir in $manager_paths; do
	if [ -d "$dir" ]; then
		for bin in $need_gone; do
			rm -f "$dir/$bin"
		done
	fi
done
