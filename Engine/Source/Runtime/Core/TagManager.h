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
		 * @brief	文字列がタブとして有効化(英数字とアンダースコアのみか)判定します。
		 */
		bool IsValidTagName(const std::string& tag) const
		{
			if (tag.empty()) return false;
			return std::all_of(tag.begin(), tag.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; });
		}

		/**
		 * @brief	システムが保護しているタグかどうか判定します(削除・変更不可)。
		 */
		bool IsProtectedTag(const std::string& tag) const
		{
			return tag == "Untagged" || tag == "Player" || tag == "Enemy" || tag == "MainCamera";
		}

		/**
		 * @brief	新しいタグを追加します。
		 * @param	tag 追加するタグ文字列
		 * @return	true 追加成功 (または既に存在) / false 不正な文字列
		 */
		bool AddTag(const std::string& tag)
		{
			if (!IsValidTagName(tag)) return false;
			if (HasTag(tag)) return true;

			m_Tags.push_back(tag);
			SortTags();
			return true;
		}

		/**
		 * @brief	指定したタグを削除します。(保護されたタグは削除不可)
		 * @param	tag 削除するタグ文字列
		 * @return	true 削除成功 / false 削除失敗 (存在しない、または保護されたタグ)
		 */
		bool RemoveTag(const std::string& tag)
		{
			if (IsProtectedTag(tag)) return false;

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

		/**
		 * @brief	"Untagged" を必ず先頭にし、残りをアルファベット順にソートします。
		 */
		void SortTags()
		{
			if (m_Tags.empty()) return;

			auto it = std::find(m_Tags.begin(), m_Tags.end(), "Untagged");
			if (it != m_Tags.end() && it != m_Tags.begin())
			{
				std::swap(m_Tags[0], *it);
			}

			if (m_Tags.size() > 1)
			{
				std::sort(m_Tags.begin() + 1, m_Tags.end());
			}
		}

	private:
		std::vector<std::string> m_Tags;
	};
}
