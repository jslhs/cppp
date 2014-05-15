#pragma once

#include <iostream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace console
{
	enum text_colors
	{
		color_black
		, color_white
		, color_red
		, color_green
		, color_blue
	};

	class text_attr
	{
	public:
		text_attr()
			: _attr(0)
		{
		}

		text_attr(text_colors foreground_color)
		{
			set_foreground_color(foreground_color);
		}

		explicit text_attr(int foreground_color)
		{
			set_foreground_color(foreground_color);
		}

		text_attr(int foreground_color, int background_color)
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

	static const text_attr text_red(color_red);
	static const text_attr text_green(color_green);
	static const text_attr text_blue(color_blue);
	static const text_attr text_white(color_white);

	std::ostream &operator<<(std::ostream &os, const text_attr &attr)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
		return os;
	}
}
