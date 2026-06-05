
export module heliox::log;
import std;


class Logger
{
public:

	template<typename... Args>
	export inline void info(std::string_view info, Args&&... args)
	{
		auto formatted_info = std::vformat(info, std::make_format_args(std::forward<Args>(args)...));
		std::println(stdout, "[info]: {}", formatted_info);
	}

};

