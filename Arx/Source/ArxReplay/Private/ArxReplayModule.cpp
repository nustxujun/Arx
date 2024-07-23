
#include "Modules/ModuleManager.h"


#if WITH_EDITOR
#include "ReplayButtonCommands.h"
#include "ReplayButtonStyle.h"
#include "ToolMenus.h"
#include "ArxReplayWindow.h"
#endif

class FArxReplayModule : public IModuleInterface
{
public:
#if WITH_EDITOR

	/** IModuleInterface implementation */
	virtual void StartupModule() override
    {

        // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
        
        FReplayButtonStyle::Initialize();
        FReplayButtonStyle::ReloadTextures();

        FReplayButtonCommands::Register();
        
        PluginCommands = MakeShareable(new FUICommandList);

        PluginCommands->MapAction(
            FReplayButtonCommands::Get().OpenPluginWindow,
            FExecuteAction::CreateRaw(this, &FArxReplayModule::PluginButtonClicked),
            FCanExecuteAction());

        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FArxReplayModule::RegisterMenus));

        FGlobalTabmanager::Get()->RegisterTabSpawner(TEXT("ReplayTool"), FOnSpawnTab::CreateLambda([this](auto Args) {
            return SNew(SDockTab).TabRole(ETabRole::NomadTab)
                [
                    SNew( ArxReplayWindow)
                ];
            })).SetMenuType(ETabSpawnerMenuType::Hidden);

    }
	virtual void ShutdownModule() override
    {

        // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
        // we call this function before unloading the module.

        UToolMenus::UnRegisterStartupCallback(this);

        UToolMenus::UnregisterOwner(this);

        FReplayButtonStyle::Shutdown();

        FReplayButtonCommands::Unregister();
    }

	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked()
	{
        FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("ReplayTool")));
    }
private:

	void RegisterMenus()
    {
        // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
        FToolMenuOwnerScoped OwnerScoped(this);

        {
            UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
            {
                FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
                Section.AddMenuEntryWithCommandList(FReplayButtonCommands::Get().OpenPluginWindow, PluginCommands);
            }
        }

        {
            UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
            {
                FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
                {
                    FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FReplayButtonCommands::Get().OpenPluginWindow));
                    Entry.SetCommandList(PluginCommands);
                }
            }
        }
    }


private:
	TSharedPtr<class FUICommandList> PluginCommands;
#endif

};



IMPLEMENT_MODULE(FArxReplayModule, ArxReplay);