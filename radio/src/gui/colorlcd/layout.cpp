/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "view_main.h"
#include "layouts/trims.h"
#include "layouts/sliders.h"

std::list<const LayoutFactory *> & getRegisteredLayouts()
{
  static std::list<const LayoutFactory *> layouts;
  return layouts;
}

void registerLayout(const LayoutFactory * factory)
{
  TRACE("register layout %s", factory->getId());
  getRegisteredLayouts().push_back(factory);
}

const LayoutFactory * getLayoutFactory(const char * name)
{
  auto it = getRegisteredLayouts().cbegin();
  for (; it != getRegisteredLayouts().cend(); ++it) {
    if (!strcmp(name, (*it)->getId())) {
      return (*it);
    }
  }
  return nullptr;
}

//
// Loads a layout, but does not attach it to any window
//
Layout * loadLayout(const char * name, Layout::PersistentData * persistentData)
{
  const LayoutFactory * factory = getLayoutFactory(name);
  if (factory) {
    return factory->load(persistentData);
  }
  return nullptr;
}

//
// Detaches and deletes all custom screens
//
void deleteCustomScreens()
{
  for (auto& screen : customScreens) {
    if (screen) {
      screen->deleteLater();
      screen = nullptr;
    }
  }
}

extern const LayoutFactory * defaultLayout;

void loadDefaultLayout()
{
  auto& screen = customScreens[0];
  auto& screenData = g_model.screenData[0];

  if (screen == nullptr && defaultLayout != nullptr) {

    strcpy(screenData.LayoutId, defaultLayout->getId());
    screen = defaultLayout->create(&screenData.layoutData);
    //
    // TODO:
    // -> attach a few default widgets
    //    - ModelBmp
    //    - Timer
    //    - ???
    //
    if (screen) {
      screen->attach(ViewMain::instance());
    }
  }
}

//
// Loads and attaches all configured custom screens
//
void loadCustomScreens()
{
  for (unsigned int i = 0; i < MAX_CUSTOM_SCREENS; i++) {

    auto& screen = customScreens[i];
    screen = loadLayout(g_model.screenData[i].LayoutId,
                        &g_model.screenData[i].layoutData);

    if (screen) {
      screen->attach(ViewMain::instance());
    }
  }
}

//
// Creates a new customer screen from factory:
//  - the old screen is detached & deleted (including children)
//  - new screen is configured into g_model
//  - the new screen is returned (not attached)
//
Layout* createCustomScreen(const LayoutFactory* factory, unsigned customScreenIndex)
{
  if (!factory || (customScreenIndex >= MAX_CUSTOM_SCREENS))
    return nullptr;

  if (customScreens[customScreenIndex]) {
    customScreens[customScreenIndex]->deleteLater(true, false);
    delete customScreens[customScreenIndex];
  }

  auto screen = factory->create(&g_model.screenData[customScreenIndex].layoutData);
  customScreens[customScreenIndex] = screen;

  if (screen) {
    auto dst = g_model.screenData[customScreenIndex].LayoutId;
    auto src = factory->getId();
    strncpy(dst, src, sizeof(CustomScreenData::LayoutId));

    return screen;
  }

  return nullptr;
}

void disposeCustomScreen(unsigned idx)
{
  // move custom screen data
  if (idx >= MAX_CUSTOM_SCREENS) {
    return;
  }

  auto dst = &g_model.screenData[idx];
  auto src = dst + 1;
  auto len = sizeof(CustomScreenData) * (MAX_CUSTOM_SCREENS - idx - 1);
  memmove(dst, src, len);

  dst = &g_model.screenData[MAX_CUSTOM_SCREENS - 1];
  len = sizeof(CustomScreenData);
  memset(dst, 0, len);
}

void Layout::decorate()
{
  // Check if deco setting are still up-to-date
  uint8_t checkSettings =
    (hasTopbar() ? 1 << 0 : 0) |
    (hasSliders() ? 1 << 1 : 0) |
    (hasTrims() ? 1 << 2 : 0) |
    (hasFlightMode() ? 1 << 3 : 0);

  if (checkSettings == decorationSettings) {
    // everything ok, exit!
    return;
  }

  // Save settings
  decorationSettings = checkSettings;

  auto viewMain = ViewMain::instance();

  // Set visible decoration
  viewMain->setTopbarVisible(hasTopbar());
  viewMain->setSlidersVisible(hasSliders());
  viewMain->setTrimsVisible(hasTrims());
  viewMain->setFlightModeVisible(hasFlightMode());

  // Re-compute positions
  viewMain->adjustDecoration();

  // and update relevant windows
  updateZones();
  invalidate();
}

rect_t Layout::getMainZone() const
{
  return ViewMain::instance()->getMainZone();
}    
