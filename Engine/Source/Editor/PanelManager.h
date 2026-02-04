/*****************************************************************//**
 * @file	PanelManager.h
 * @brief	エディタパネルのファクトリーと自動登録システム。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Panels/EditorPanel.h"

namespace Span
{
	/**
	 * @class	PanelManager
	 * @brief	🗂️ 全エディタパネルの生成と管理を行うクラス。
	 * 
	 * @details
	 * 各パネルクラスは、cppファイル内で `AutoRegisterPanel` を定義することで
	 * 自動的にこのマネージャーに登録されます。
	 * これにより、マネージャー側のコードを書き換えることなく新しいパネルを追加できます。
	 */
	class PanelManager
	{
	public:
		using PanelFactory = std::function<std::shared_ptr<EditorPanel>()>;

		/**
		 * @brief	パネル生成関数を登録します。
		 * @param	name パネル識別名
		 * @param	factory 生成用ラムダ式
		 */
		static void RegisterPanel(const std::string& name, PanelFactory factory)
		{
			GetRegistry()[name] = factory;
		}

		/**
		 * @brief	登録されている全てのパネルをインスタンス化して返します。
		 * @return	生成されたパネルのリスト
		 */
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
		/// @brief	静的マップへのアクセサ
		static std::map<std::string, PanelFactory>& GetRegistry()
		{
			static std::map<std::string, PanelFactory> registry;
			return registry;
		}
	};

	/**
	 * @struct	AutoRegisterPanel
	 * @brief	🖊 パネルを自動登録するためのヘルパー構造体。
	 * 
	 * @tparam	T 登録するパネルクラス (EditorPanel継承)
	 * 
	 * @details
	 * cppファイルでグローバル変数として定義することで、main関数前にコンストラクタが走り、登録が行われます。
	 * 
	 * @code	{.cpp}
	 * // HierarchyPanel.cpp
	 * static AutoRegisterPanel<HierarchyPanel> _reg("Hierarchy");
	 */
	template <typename T>
	struct AutoRegisterPanel
	{
		AutoRegisterPanel(const std::string& name)
		{
			PanelManager::RegisterPanel(name, []() { return std::make_shared<T>(); });
		}
	};
}

