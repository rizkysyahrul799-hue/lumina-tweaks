import { createRouter, createWebHashHistory } from 'vue-router'

import Home from '../views/Home.vue'
import AppList from '../views/AppList.vue'
import Tweak from '../views/Tweak.vue'
import AppSettings from '../views/AppSettings.vue'
import GeneralSettings from '../views/GeneralSettings.vue'
import LanguageSelection from '../views/LanguageSelection.vue'

const routes = [
  // Halaman Tab Utama
  { path: '/', name: 'Home', component: Home, alias: ['/home'] },
  { path: '/applist', name: 'AppList', component: AppList, alias: ['/games', '/app-list', '/apps'] },
  { path: '/settings', name: 'Tweak', component: Tweak, alias: ['/tweak', '/tweaks'] },

  // Detail Pengaturan Aplikasi
  { 
    path: '/app-settings/:package*', 
    name: 'AppSettings', 
    component: AppSettings, 
    alias: [
      '/applist/:package+', 
      '/games/:package+', 
      '/game-settings/:package+', 
      '/game/:package+'
    ] 
  },

  // Sub-Pengaturan Aktif
  { 
    path: '/general-settings', 
    name: 'GeneralSettings', 
    component: GeneralSettings, 
    alias: ['/tweaks/general', '/tweak/general', '/settings/general', '/general'] 
  },
  { 
    path: '/language', 
    name: 'LanguageSelection', 
    component: LanguageSelection, 
    alias: ['/tweaks/language', '/tweak/language', '/settings/language', '/language-selection'] 
  },

{
  path: '/tweaks/advanced-controls',
  name: 'AdvancedControls',
  component: () => import('@/views/AdvancedControls.vue')
}
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
})

export default router
