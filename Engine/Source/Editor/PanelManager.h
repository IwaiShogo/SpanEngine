#pragma once
#include "Panels/EditorPanel.h"

namespace Span
{
	class PanelManager
	{
	public:
		using PanelFactory = std::function<std::shared_ptr<EditorPanel>()>;

		// パネル登録用関数
		static void RegisterPanel(const std::string& name, PanelFactory factory)
		{
			GetRegistry()[name] = factory;
		}

		// 全パネルのインスタンス化
		static std::vector<std::shared_ptr<EditorPanel>> CreateAllPanels()
		{
			std::vector<std::shared_ptr<EditorPanel>> panels;
			for (auto& pair : GetRegistry())
			{
				panels.push_back(pair.second());
			}
			return panels;
		}

	private:
		// 静的マップへのアクセサ
		static std::map<std::string, PanelFactory>& GetRegistry()
		{
			static std::map<std::string, PanelFactory> registry;
			return registry;
		}
	};

	// 自動登録用ヘルパー構造体
	template <typename T>
	struct AutoRegisterPanel
	{
		AutoRegisterPanel(const std::string& name)
		{
			PanelManager::RegisterPanel(name, []() { return std::make_shared<T>(); });
		}
	};
}

