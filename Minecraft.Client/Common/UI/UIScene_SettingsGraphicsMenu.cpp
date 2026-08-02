#include "stdafx.h"
#include "UI.h"
#include "UIScene_SettingsGraphicsMenu.h"
#include "../../Minecraft.h"
#include "../../Options.h"
#include "../../GameRenderer.h"

#ifdef _WINDOWS64
extern bool g_bVSync;
extern void SetExclusiveFullscreen(bool enabled);
#endif

namespace
{
    constexpr int FOV_MIN = 70;
    constexpr int FOV_MAX = 110;
    constexpr int FOV_SLIDER_MAX = 100;

	int ClampFov(int value)
	{
		if (value < FOV_MIN) return FOV_MIN;
		if (value > FOV_MAX) return FOV_MAX;
		return value;
	}

    [[maybe_unused]]
    int FovToSliderValue(float fov)
	{
		const int clampedFov = ClampFov(static_cast<int>(fov + 0.5f));
		return ((clampedFov - FOV_MIN) * FOV_SLIDER_MAX) / (FOV_MAX - FOV_MIN);
	}

	int sliderValueToFov(int sliderValue)
	{
		if (sliderValue < 0) sliderValue = 0;
		if (sliderValue > FOV_SLIDER_MAX) sliderValue = FOV_SLIDER_MAX;
		return FOV_MIN + ((sliderValue * (FOV_MAX - FOV_MIN)) / FOV_SLIDER_MAX);
	}
}

int UIScene_SettingsGraphicsMenu::LevelToDistance(int level)
{
	if(level <= 0) return 0;
	if(level > 31) level = 31;
	return level + 1;
}

int UIScene_SettingsGraphicsMenu::DistanceToLevel(int dist)
{
	if(dist <= 0) return 0;
	int level = dist - 1;
	if(level < 0) level = 0;
	if(level > 31) level = 31;
	return level;
}

UIScene_SettingsGraphicsMenu::UIScene_SettingsGraphicsMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();
	Minecraft* pMinecraft = Minecraft::GetInstance();
	
	m_bNotInGame=(Minecraft::GetInstance()->level==nullptr);

	m_checkboxClouds.init(app.GetString(IDS_CHECKBOX_RENDER_CLOUDS),eControl_Clouds,(app.GetGameSettings(m_iPad,eGameSetting_Clouds)!=0));
	m_checkboxBedrockFog.init(app.GetString(IDS_CHECKBOX_RENDER_BEDROCKFOG),eControl_BedrockFog,(app.GetGameSettings(m_iPad,eGameSetting_BedrockFog)!=0));
	m_checkboxCustomSkinAnim.init(app.GetString(IDS_CHECKBOX_CUSTOM_SKIN_ANIM),eControl_CustomSkinAnim,(app.GetGameSettings(m_iPad,eGameSetting_CustomSkinAnim)!=0));
	m_checkboxVSync.init(L"VSync",eControl_VSync,(app.GetGameSettings(m_iPad,eGameSetting_VSync)!=0));
	m_checkboxExclusiveFullscreen.init(L"Fullscreen",eControl_ExclusiveFullscreen,(app.GetGameSettings(m_iPad,eGameSetting_ExclusiveFullscreen)!=0));


	WCHAR TempString[256];

	swprintf(TempString, 256, L"Render Distance: %d",app.GetGameSettings(m_iPad,eGameSetting_RenderDistance));	
	m_sliderRenderDistance.init(TempString,eControl_RenderDistance,0,31,DistanceToLevel(app.GetGameSettings(m_iPad,eGameSetting_RenderDistance)));
	
	swprintf( TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_GAMMA ),app.GetGameSettings(m_iPad,eGameSetting_Gamma));	
	m_sliderGamma.init(TempString,eControl_Gamma,0,100,app.GetGameSettings(m_iPad,eGameSetting_Gamma));

    const int initialFovSlider = app.GetGameSettings(m_iPad, eGameSetting_FOV);
	const int initialFovDeg = sliderValueToFov(initialFovSlider);
	swprintf(TempString, 256, L"FOV: %d", initialFovDeg);
	m_sliderFOV.init(TempString, eControl_FOV, 0, FOV_SLIDER_MAX, initialFovSlider);
	
	swprintf( TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_INTERFACEOPACITY ),app.GetGameSettings(m_iPad,eGameSetting_InterfaceOpacity));	
	m_sliderInterfaceOpacity.init(TempString,eControl_InterfaceOpacity,0,100,app.GetGameSettings(m_iPad,eGameSetting_InterfaceOpacity));

	doHorizontalResizeCheck();

#ifndef _WINDOWS64
	// VSync and Exclusive Fullscreen are only available on PC
	removeControl(&m_checkboxVSync, true);
	removeControl(&m_checkboxExclusiveFullscreen, true);
#else
	// The SWF's original focus chain skips VSync, Fullscreen, and RenderDistance
	// (CustomSkinAnim -> Gamma). Rewire the navigation so all controls are reachable:
	// CustomSkinAnim -> VSync -> Fullscreen -> RenderDistance -> Gamma
	{
		IggyName navDown = registerFastName(L"m_objNavDown");
		IggyName navUp   = registerFastName(L"m_objNavUp");

		IggyValueSetStringUTF8RS(m_checkboxCustomSkinAnim.getIggyValuePath(), navDown, nullptr, "VSync", -1);

		IggyValueSetStringUTF8RS(m_checkboxVSync.getIggyValuePath(), navUp, nullptr, "CustomSkinAnim", -1);
		IggyValueSetStringUTF8RS(m_checkboxVSync.getIggyValuePath(), navDown, nullptr, "ExclusiveFullscreen", -1);

		IggyValueSetStringUTF8RS(m_checkboxExclusiveFullscreen.getIggyValuePath(), navUp, nullptr, "VSync", -1);
		IggyValueSetStringUTF8RS(m_checkboxExclusiveFullscreen.getIggyValuePath(), navDown, nullptr, "RenderDistance", -1);

		IggyValueSetStringUTF8RS(m_sliderRenderDistance.getIggyValuePath(), navUp, nullptr, "ExclusiveFullscreen", -1);
	}
#endif

	const bool bInGame=(Minecraft::GetInstance()->level!=nullptr);
	const bool bIsPrimaryPad=(ProfileManager.GetPrimaryPad()==m_iPad);
	// if we're not in the game, we need to use basescene 0
	if(bInGame)
	{
#ifndef _WINDOWS64
		// Console splitscreen: non-host and non-primary players can't change world-level settings
		if(bIsPrimaryPad)
		{
			if(!g_NetworkManager.IsHost())
			{
				removeControl(&m_checkboxBedrockFog, true);
			}
		}
		else
		{
			removeControl(&m_checkboxBedrockFog, true);
			removeControl(&m_checkboxCustomSkinAnim, true);
		}
#endif
	}

	if(app.GetLocalPlayerCount()>1)
	{
#if TO_BE_IMPLEMENTED
		app.AdjustSplitscreenScene(m_hObj,&m_OriginalPosition,m_iPad);
#endif
	}
}

UIScene_SettingsGraphicsMenu::~UIScene_SettingsGraphicsMenu()
{
}

wstring UIScene_SettingsGraphicsMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"SettingsGraphicsMenuSplit";
	}
	else
	{
		return L"SettingsGraphicsMenu";
	}
}

void UIScene_SettingsGraphicsMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_SettingsGraphicsMenu::updateComponents()
{
	const bool bNotInGame=(Minecraft::GetInstance()->level==nullptr);
	if(bNotInGame)
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);
		m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
	}
	else
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,false);
	
		if( app.GetLocalPlayerCount() == 1 ) m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
		else m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);

	}
}

void UIScene_SettingsGraphicsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(iPad, key, repeat, pressed, released);
	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			// check the checkboxes
			app.SetGameSettings(m_iPad,eGameSetting_Clouds,m_checkboxClouds.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_BedrockFog,m_checkboxBedrockFog.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_CustomSkinAnim,m_checkboxCustomSkinAnim.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_VSync,m_checkboxVSync.IsChecked()?1:0);
			app.SetGameSettings(m_iPad,eGameSetting_ExclusiveFullscreen,m_checkboxExclusiveFullscreen.IsChecked()?1:0);
#ifdef _WINDOWS64
			g_bVSync = m_checkboxVSync.IsChecked();
			SetExclusiveFullscreen(m_checkboxExclusiveFullscreen.IsChecked());
#endif

			navigateBack();
			handled = true;
		}
		break;
	case ACTION_MENU_OK:
#ifdef __ORBIS__
	case ACTION_MENU_TOUCHPAD_PRESS:
#endif
		sendInputToMovie(key, repeat, pressed, released);
		break;
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_LEFT:
	case ACTION_MENU_RIGHT:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_SettingsGraphicsMenu::handleSliderMove(F64 sliderId, F64 currentValue)
{
	WCHAR TempString[256];
	const int value = static_cast<int>(currentValue);
	switch(static_cast<int>(sliderId))
	{
	case eControl_RenderDistance:
		{
			m_sliderRenderDistance.handleSliderMove(value);

			const int dist = LevelToDistance(value);

			app.SetGameSettings(m_iPad,eGameSetting_RenderDistance,dist);

			const Minecraft* mc = Minecraft::GetInstance();
			mc->options->viewDistance = value;
			swprintf(TempString,256,L"Render Distance: %d",dist);
			m_sliderRenderDistance.setLabel(TempString);
		}
		break;

	case eControl_Gamma:
		m_sliderGamma.handleSliderMove(value);
		
		app.SetGameSettings(m_iPad,eGameSetting_Gamma,value);
		swprintf( TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_GAMMA ),value);
		m_sliderGamma.setLabel(TempString);

		break;

	case eControl_FOV:
		{
			m_sliderFOV.handleSliderMove(value);
			const Minecraft* pMinecraft = Minecraft::GetInstance();
			const int fovValue = sliderValueToFov(value);
			pMinecraft->gameRenderer->SetFovVal(static_cast<float>(fovValue));
			app.SetGameSettings(m_iPad, eGameSetting_FOV, value);
			swprintf(TempString, 256, L"FOV: %d", fovValue);
			m_sliderFOV.setLabel(TempString);
		}
		break;

	case eControl_InterfaceOpacity:
		m_sliderInterfaceOpacity.handleSliderMove(value);
		
		app.SetGameSettings(m_iPad,eGameSetting_InterfaceOpacity,value);
		swprintf( TempString, 256, L"%ls: %d%%", app.GetString( IDS_SLIDER_INTERFACEOPACITY ),value);	
		m_sliderInterfaceOpacity.setLabel(TempString);

		break;
	}
}
