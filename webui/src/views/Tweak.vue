<template>
  <div 
    class="page settings-page w-full min-h-screen h-screen overflow-y-scroll bg-surface pb-44 select-none"
    style="touch-action: pan-y !important; -webkit-overflow-scrolling: touch !important;"
  >
    <!-- Header Sticky -->
    <div class="sticky top-0 z-10 bg-surface/80 backdrop-blur-md">
      <div class="max-w-3xl mx-auto p-5 pb-3">
        <div class="flex justify-between items-center text-on-surface">
          <h1 class="text-xl font-semibold">{{ $t('settings_page.title') }}</h1>
          
          <button 
            @click="$router.push('/tweaks/general')" 
            class="p-2 -mr-2 rounded-full text-on-surface hover:bg-on-surface/10 transition-colors cursor-pointer"
            title="Pengaturan"
          >
            <IconDots class="w-6 h-6 text-on-surface" />
          </button>
        </div>
      </div>
    </div>

    <!-- Content Area -->
    <div class="scrollbar-hidden max-w-3xl mx-auto p-5 py-1 space-y-5">
      
      <!-- ==================== KATEGORI 1: PENGATURAN CPU & SUHU ==================== -->
      <div>
        <div class="px-4 py-1 mb-1.5">
          <h2 class="text-xs font-bold text-primary tracking-wide">
            {{ $t('settings_page.section.cpu_thermal') }}
          </h2>
        </div>

        <div class="space-y-[2px]">
          <!-- CPU Freq Slider Card -->
          <div class="bg-surface-container-low rounded-t-[24px] rounded-b-[6px] p-4 transition-all">
            <div class="flex items-center gap-4 min-w-0 mb-2">
              <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
                <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5 text-primary" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <rect x="4" y="4" width="16" height="16" rx="2" ry="2"/>
                  <rect x="9" y="9" width="6" height="6"/>
                  <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
                  <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
                  <line x1="20" y1="9" x2="23" y2="9"/><line x1="15" y1="15" x2="23" y2="15"/>
                  <line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="15" x2="4" y2="15"/>
                </svg>
              </div>

              <div class="flex-1 min-w-0">
                <h3 class="text-sm font-medium text-on-surface">
                  {{ $t('settings_page.cpu_freq.title') }}
                </h3>
              </div>
            </div>

            <div class="px-1 mt-1">
              <div class="relative w-full h-6 flex items-center">
                <div class="absolute inset-x-0 h-3.5 bg-on-surface/15 rounded-full overflow-hidden pointer-events-none">
                  <div 
                    class="h-full bg-primary/35 transition-all duration-150"
                    :style="{ width: `calc(3px + (100% - 6px) * ${(localCpuLimit - 50) / 50})` }"
                  ></div>
                </div>

                <div class="absolute inset-x-0 inset-y-0 pointer-events-none">
                  <span 
                    v-for="(step, index) in [50, 60, 70, 80, 90, 100]" 
                    :key="step"
                    class="absolute top-1/2 -translate-y-1/2 -translate-x-1/2 w-1 h-1 rounded-full transition-colors duration-150 z-10"
                    :style="{ left: `calc(3px + (100% - 6px) * ${index / 5})` }"
                    :class="step <= localCpuLimit ? 'bg-primary' : 'bg-on-surface/40'"
                  ></span>
                </div>

                <input
                  type="range"
                  min="50"
                  max="100"
                  step="10"
                  v-model.number="localCpuLimit"
                  @change="onCpuLimitChange"
                  class="custom-slider absolute inset-0 w-full h-full bg-transparent appearance-none cursor-pointer z-20"
                />
              </div>

              <div class="flex justify-between text-[11px] font-medium text-on-surface-variant px-0.5 mt-1">
                <span>50%</span>
                <span :class="localCpuLimit === 100 ? 'text-primary font-semibold' : ''">{{ $t('settings_page.cpu_freq.disabled') }}</span>
              </div>
            </div>
          </div>

          <!-- Disable Thermal Card -->
          <div class="bg-surface-container-low rounded-t-[6px] rounded-b-[24px] flex items-center justify-between px-5 py-4 transition-all">
            <div class="flex items-center gap-4 min-w-0 flex-1">
              <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
                <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5 text-primary" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/>
                </svg>
              </div>

              <div class="flex-1 min-w-0 pr-2">
                <h3 class="text-sm font-medium text-on-surface">
                  {{ $t('settings_page.disable_thermal.title') }}
                </h3>
                <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                  {{ $t('settings_page.disable_thermal.description') }}
                </p>
              </div>
            </div>

            <UiToggle
              :model-value="isDisableThermalEnabled"
              @update:model-value="toggleDisableThermal"
              class="shrink-0 ms-3"
            />
          </div>
        </div>
      </div>

      <!-- ==================== KATEGORI 2: REFRESH RATE & RENDER ==================== -->
      <div>
        <div class="px-4 py-1 mb-1.5">
          <h2 class="text-xs font-bold text-primary tracking-wide">
            {{ $t('settings_page.section.display_render') }}
          </h2>
        </div>

        <div class="grid grid-cols-2 gap-2.5">
          <!-- Refresh Rate Card (MATI & GELAP SESUAI SCREENSHOT) -->
          <div class="bg-surface-container-lowest/50 opacity-40 rounded-[24px] p-4 flex items-center gap-3 pointer-events-none select-none">
            <div class="w-10 h-10 rounded-xl bg-surface-container-high/40 text-on-surface-variant/60 flex items-center justify-center shrink-0">
              <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <rect x="2" y="3" width="20" height="14" rx="2" ry="2"/>
                <line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/>
              </svg>
            </div>
            <div class="min-w-0 flex-1 flex flex-col justify-center">
              <h3 class="text-sm font-medium text-on-surface-variant/70 leading-tight">{{ $t('settings_page.refresh_rate.title') }}</h3>
              <p class="text-xs text-on-surface-variant/50 font-medium mt-1">{{ selectedRefreshRate }}</p>
            </div>
          </div>

          <!-- Render Engine Card (AKTIF) -->
          <div class="bg-surface-container-low rounded-[24px] p-4 flex items-center gap-3 cursor-pointer hover:bg-surface-container transition-all active:scale-[0.99]" @click="showRenderModal = true">
            <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
              <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5 text-primary" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
              </svg>
            </div>
            <div class="min-w-0 flex-1 flex flex-col justify-center">
              <h3 class="text-sm font-medium text-on-surface leading-tight">{{ $t('settings_page.render_engine.title') }}</h3>
              <p class="text-xs text-primary font-semibold mt-1">{{ selectedRender }}</p>
            </div>
          </div>
        </div>
      </div>

      <!-- ==================== KATEGORI 3: PENGATURAN TAMBAHAN ==================== -->
      <div>
        <div class="px-4 py-1 mb-1.5">
          <h2 class="text-xs font-bold text-primary tracking-wide">
            {{ $t('settings_page.section.additional') }}
          </h2>
        </div>

        <div class="space-y-[2px]">
          <!-- Mode Lite -->
          <div class="bg-surface-container-low rounded-t-[24px] rounded-b-[6px] flex items-center justify-between px-5 py-4 transition-all">
            <div class="flex items-center gap-4 min-w-0 flex-1">
              <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
                <IconTune class="w-5 h-5 text-primary" />
              </div>

              <div class="flex-1 min-w-0 pr-2">
                <h3 class="text-sm font-medium text-on-surface">
                  {{ $t('settings_page.lite_mode.title') }}
                </h3>
                <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                  {{ $t('settings_page.lite_mode.description') }}
                </p>
              </div>
            </div>

            <UiToggle
              :model-value="isLiteModeEnabled"
              @update:model-value="toggleLiteMode"
              class="shrink-0 ms-3"
            />
          </div>

          <!-- RAM Optimization -->
          <div class="bg-surface-container-low rounded-[6px] flex items-center justify-between px-5 py-4 transition-all">
            <div class="flex items-center gap-4 min-w-0 flex-1">
              <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
                <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5 text-primary" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M6 19v2M10 19v2M14 19v2M18 19v2M6 3v2M10 3v2M14 3v2M18 3v2M3 6h2M3 10h2M3 14h2M3 18h2M19 6h2M19 10h2M19 14h2M19 18h2"/>
                  <rect x="5" y="5" width="14" height="14" rx="2"/>
                </svg>
              </div>

              <div class="flex-1 min-w-0 pr-2">
                <h3 class="text-sm font-medium text-on-surface">
                  {{ $t('settings_page.ram_opt.title') }}
                </h3>
                <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                  {{ $t('settings_page.ram_opt.description') }}
                </p>
              </div>
            </div>

            <UiToggle
              :model-value="isRamTweaksEnabled"
              @update:model-value="toggleRamTweaks"
              class="shrink-0 ms-3"
            />
          </div>

          <!-- TCP BBR Optimization -->
          <div class="bg-surface-container-low rounded-[6px] flex items-center justify-between px-5 py-4 transition-all">
            <div class="flex items-center gap-4 min-w-0 flex-1">
              <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
                <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5 text-primary" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M5 12h14M12 5l7 7-7 7"/>
                </svg>
              </div>

              <div class="flex-1 min-w-0 pr-2">
                <h3 class="text-sm font-medium text-on-surface">
                  {{ $t('settings_page.tcp_bbr.title') }}
                </h3>
                <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                  {{ $t('settings_page.tcp_bbr.description') }}
                </p>
              </div>
            </div>

            <UiToggle
              :model-value="isTcpBbrEnabled"
              @update:model-value="toggleTcpBbr"
              class="shrink-0 ms-3"
            />
          </div>

          <!-- MediaTek Anti-Throttling -->
          <div class="bg-surface-container-low rounded-t-[6px] rounded-b-[24px] flex items-center justify-between px-5 py-4 transition-all">
            <div class="flex items-center gap-4 min-w-0 flex-1">
              <div class="w-10 h-10 rounded-xl bg-surface-container-high text-primary flex items-center justify-center shrink-0">
                <svg xmlns="http://www.w3.org/2000/svg" class="w-5 h-5 text-primary" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/>
                </svg>
              </div>

              <div class="flex-1 min-w-0 pr-2">
                <h3 class="text-sm font-medium text-on-surface">
                  {{ $t('settings_page.mtk_throttling.title') }}
                </h3>
                <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                  {{ $t('settings_page.mtk_throttling.description') }}
                </p>
              </div>
            </div>

            <UiToggle
              :model-value="isMtkTweaksEnabled"
              @update:model-value="toggleMtkTweaks"
              class="shrink-0 ms-3"
            />
          </div>
        </div>
      </div>

    </div>

    <!-- Modal Refresh Rate (Tetap tersimpan di kode tapi tidak ter-trigger) -->
    <div v-if="showRefreshModal" class="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4" @click.self="showRefreshModal = false">
      <div class="bg-surface rounded-2xl p-5 w-full max-w-xs space-y-3 shadow-xl border border-on-surface/10">
        <h3 class="text-base font-semibold text-on-surface">{{ $t('settings_page.refresh_rate.modal_title') }}</h3>
        <div class="space-y-1">
          <button 
            v-for="hz in refreshOptions" 
            :key="hz"
            @click="selectRefreshRate(hz)"
            class="w-full text-left px-4 py-2.5 rounded-xl text-sm font-medium transition-colors flex justify-between items-center cursor-pointer"
            :class="selectedRefreshRate === hz ? 'bg-primary-container text-on-primary-container' : 'text-on-surface hover:bg-on-surface/5'"
          >
            <span>{{ hz }}</span>
            <span v-if="selectedRefreshRate === hz" class="text-xs">✓</span>
          </button>
        </div>
      </div>
    </div>

    <!-- Modal Render Engine -->
    <div v-if="showRenderModal" class="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4" @click.self="showRenderModal = false">
      <div class="bg-surface rounded-2xl p-5 w-full max-w-xs space-y-3 shadow-xl border border-on-surface/10">
        <h3 class="text-base font-semibold text-on-surface">{{ $t('settings_page.render_engine.modal_title') }}</h3>
        <div class="space-y-1">
          <button 
            v-for="renderer in renderOptions" 
            :key="renderer"
            @click="selectRender(renderer)"
            class="w-full text-left px-4 py-2.5 rounded-xl text-sm font-medium transition-colors flex justify-between items-center cursor-pointer"
            :class="selectedRender === renderer ? 'bg-primary-container text-on-primary-container' : 'text-on-surface hover:bg-on-surface/5'"
          >
            <span>{{ renderer }}</span>
            <span v-if="selectedRender === renderer" class="text-xs">✓</span>
          </button>
        </div>
      </div>
    </div>

  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useLuminaConfigStore } from '@/stores/LuminaConfig'
import { exec } from '@/utils/exec'

import IconDots from '@/components/icons/IconDots.vue'
import UiToggle from '@/components/ui/UiToggle.vue'
import IconTune from '@/components/icons/IconTune.vue'

const router = useRouter()
const luminaConfigStore = useLuminaConfigStore()

const localCpuLimit = ref(100)

const showRefreshModal = ref(false)
const showRenderModal = ref(false)

const selectedRefreshRate = ref('60 Hz')
const selectedRender = ref('Vulkan')

const refreshOptions = ref(['Auto', '60 Hz', '90 Hz', '120 Hz'])
const renderOptions = ['Auto', 'Vulkan', 'OpenGL', 'Skia']

onMounted(async () => {
  if (!luminaConfigStore.isLoaded) {
    await luminaConfigStore.loadConfig()
  }
  if (luminaConfigStore.cpuLimit !== undefined) {
    localCpuLimit.value = luminaConfigStore.cpuLimit
  }
  if (luminaConfigStore.refreshRate) {
    selectedRefreshRate.value = luminaConfigStore.refreshRate
  }
  if (luminaConfigStore.renderEngine) {
    selectedRender.value = luminaConfigStore.renderEngine
  }

  try {
    const res = await exec('luminad get_supported_hz')
    if (res && res.trim()) {
      const parsed = JSON.parse(res.trim())
      if (Array.isArray(parsed) && parsed.length > 0) {
        refreshOptions.value = ['Auto', ...parsed]
      }
    }
  } catch (e) {
    console.warn('Gagal memuat dynamic Hz, memakai fallback options:', e)
  }
})

watch(
  () => luminaConfigStore.cpuLimit,
  (newVal) => {
    if (newVal !== undefined) localCpuLimit.value = newVal
  }
)

watch(
  () => luminaConfigStore.refreshRate,
  (newVal) => {
    if (newVal) selectedRefreshRate.value = newVal
  }
)

watch(
  () => luminaConfigStore.renderEngine,
  (newVal) => {
    if (newVal) selectedRender.value = newVal
  }
)

const isLiteModeEnabled = computed(() => luminaConfigStore.isLiteModeEnabled)
const isTcpBbrEnabled = computed(() => luminaConfigStore.isTcpBbrEnabled)
const isRamTweaksEnabled = computed(() => luminaConfigStore.isRamTweaksEnabled)
const isMtkTweaksEnabled = computed(() => luminaConfigStore.isMtkTweaksEnabled)
const isDisableThermalEnabled = computed(() => luminaConfigStore.isDisableThermalEnabled)

const toggleLiteMode = (val) => luminaConfigStore.setLiteMode(val)
const toggleTcpBbr = (val) => luminaConfigStore.setTcpBbr(val)
const toggleRamTweaks = (val) => luminaConfigStore.setRamTweaks(val)
const toggleMtkTweaks = (val) => luminaConfigStore.setMtkTweaks(val)
const toggleDisableThermal = (val) => luminaConfigStore.setDisableThermal(val)

const selectRefreshRate = (hz) => {
  selectedRefreshRate.value = hz
  showRefreshModal.value = false
  if (luminaConfigStore.setRefreshRate) {
    const cleanHz = hz.replace(' Hz', '').trim()
    luminaConfigStore.setRefreshRate(cleanHz)
  }
}

const selectRender = (renderer) => {
  selectedRender.value = renderer
  showRenderModal.value = false
  if (luminaConfigStore.setRenderEngine) {
    luminaConfigStore.setRenderEngine(renderer)
  }
}

const onCpuLimitChange = () => {
  if (luminaConfigStore.setCpuLimit) {
    luminaConfigStore.setCpuLimit(localCpuLimit.value)
  }
}
</script>

<style scoped>
.settings-page {
  touch-action: pan-y !important;
  -webkit-overflow-scrolling: touch !important;
  overscroll-behavior-y: contain;
  overflow-y: scroll !important;
}

.line-clamp-2 {
  display: -webkit-box;
  line-clamp: 2;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.custom-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 6px;
  height: 20px;
  background-color: var(--color-primary, #e8def8);
  border-radius: 3px;
  cursor: pointer;
  box-shadow: 0 0 4px rgba(0, 0, 0, 0.4);
}

.custom-slider::-moz-range-thumb {
  width: 6px;
  height: 20px;
  background-color: var(--color-primary, #e8def8);
  border-radius: 3px;
  cursor: pointer;
  border: none;
  box-shadow: 0 0 4px rgba(0, 0, 0, 0.4);
}
</style>
