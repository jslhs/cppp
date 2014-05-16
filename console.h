#pragma once

#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else

using WORD = unsigned short;
enum
{
	FOREGROUND_RED = 31,
	FOREGROUND_GREEN = 32,
	FOREGROUND_BLUE = 34,
	FOREGROUND_DEFAULT = 39,
	BACKGROUND_RED = 41,
	BACKGROUND_GREEN = 42,
	BACKGROUND_BLUE = 44,
	BACKGROUND_DEFAULT = 49,
};

#endif

namespace console
{
	enum text_colors
	{
		color_black
		, color_white
		, color_red
		, color_green
		, color_blue

		, default
		, black
		, red
		, green
		, yellow
		, blue
		, magenta
		, cyan
		, light_gray
		, dark_gray
		, light_red
		, light_green
		, light_yellow
		, light_blue
		, light_magenta
		, light_cyan
		, white
	};

	class text_attr
	{
	public:
		text_attr()
			: _attr(0)
		{
		}

		text_attr(text_colors foreground_color)
			: _attr(0)
		{
			set_foreground_color(foreground_color);
		}

		explicit text_attr(int foreground_color)
			: _attr(0)
		{
			set_foreground_color(foreground_color);
		}

		text_attr(int foreground_color, int background_color)
			: _attr(0)
		{
			set_foreground_color(foreground_color);
			set_background_color(background_color);
		}

		void set_foreground_color(int color)
		{
			switch (color)
			{
			case color_white:
				_attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
				break;
			case color_black:
				_attr &= ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
				break;
			case color_red:
				_attr |= FOREGROUND_RED;
				break;
			case color_green:
				_attr |= FOREGROUND_GREEN;
				break;
			case color_blue:
				_attr |= FOREGROUND_BLUE;
				break;
			}
		}

		void set_background_color(int color)
		{
			switch (color)
			{
			case color_white:
				_attr |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
				break;
			case color_black:
				_attr &= ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
				break;
			case color_red:
				_attr |= BACKGROUND_RED;
				break;
			case color_green:
				_attr |= BACKGROUND_GREEN;
				break;
			case color_blue:
				_attr |= BACKGROUND_BLUE;
				break;
			}
		}

		operator WORD() const
		{
			return _attr;
		}

	private:
		WORD _attr;
	};

	std::ostream &operator<<(std::ostream &os, const text_attr &attr)
	{
#ifdef _WIN32
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
#else
		os << "\033[" << int(attr) << "m";
#endif
		return os;
	}

	text_attr seta(text_colors foreground_color)
	{
		return text_attr(foreground_color);
	}

	text_attr seta(text_colors foreground_color, text_colors background_color)
	{
		return text_attr(foreground_color, background_color);
	}
}
