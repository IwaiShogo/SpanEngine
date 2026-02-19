/*****************************************************************//**
 * @file	TagManager.h
 * @brief	エンジン全体で利用可能なタグ(Tag)文字列を一元管理するマネージャー。
 *
 * @details
 * 高性能なバリデーション、自動ソート、およびコアタグの保護機能を備えています。
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once

namespace Span
{
	/**
	 * @class	TagManager
	 * @brief	エンティティのタグ一覧を管理する高機能シングルトンクラス。
	 */
	class TagManager
	{
	public:
		/**
		 * @brief	TagManagerのシングルトンインスタンスを取得します。
		 * @return	TagManager& インスタンス参照
		 */
		static TagManager& Get()
		{
			static TagManager instance;
			return instance;
		}

		/**
		 * @brief	登録されているすべてのタグリストを取得します。
		 * @return	const std::vector<std::string>& タグのリスト
		 */
		const std::vector<std::string>& GetAllTags() const { return m_Tags; }

		/**
		 * @brief	新しいタグを追加します。
		 * @param	tag 追加するタグ文字列
		 * @return	true 追加成功 (または既に存在) / false 不正な文字列
		 */
		bool AddTag(const std::string& tag)
		{
			if (tag.empty()) return false;
			if (HasTag(tag)) return true;

			m_Tags.push_back(tag);
			return true;
		}

		/**
		 * @brief	指定したタグを削除します。("Untagged"は削除不可)
		 * @param	tag 削除するタグ文字列
		 * @return	true 削除成功 / false 削除失敗 (存在しない、または保護されたタグ)
		 */
		bool RemoveTag(const std::string& tag)
		{
			if (tag == "Untagged") return false;

			auto it = std::find(m_Tags.begin(), m_Tags.end(), tag);
			if (it != m_Tags.end())
			{
				m_Tags.erase(it);
				return true;
			}
			return false;
		}

		/**
		 * @brief	タグが存在するかどうかを確認します。
		 * @param	tag 確認するタグ文字列
		 * @return	true 存在する / false 存在しない
		 */
		bool HasTag(const std::string& tag) const
		{
			return std::find(m_Tags.begin(), m_Tags.end(), tag) != m_Tags.end();
		}

	private:
		TagManager()
		{
			// デフォルトで用意される基本タグ
			m_Tags = { "Untagged", "Player", "Enemy", "MainCamera" };
		}
		~TagManager() = default;

	private:
		std::vector<std::string> m_Tags;
	};
}
