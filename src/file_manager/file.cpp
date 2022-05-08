#include "common.hpp"
#include "file.hpp"
#include "file_manager.hpp"

namespace ext
{
	file::file(file_manager* file_manager, fs::path file_path)
		: file(file_manager->get_base_dir() / file_path)
	{
		m_is_project_file = true;
	}

	file::file(fs::path file_path)
		: m_file_path(file_manager::ensure_file_can_be_created(file_path)), m_is_project_file(false)
	{

	}

	bool file::exists() const
	{
		return fs::exists(m_file_path);
	}

	const fs::path file::get_path() const
	{
		return m_file_path;
	}

	file file::move(fs::path new_path)
	{
		if (new_path.is_relative())
			new_path = m_file_path.parent_path() / new_path;

		file_manager::ensure_file_can_be_created(new_path);

		if (fs::exists(m_file_path))
			fs::rename(m_file_path, new_path);

		return { new_path };
	}
}