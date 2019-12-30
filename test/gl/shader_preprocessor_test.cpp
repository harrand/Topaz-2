//
// Created by Harrand on 25/12/2019.
//

#include "test_framework.hpp"
#include "core/core.hpp"
#include "core/tz_glad/glad_context.hpp"
#include "gl/shader_preprocessor.hpp"
#include "gl/modules/include.hpp"
#include <cctype>

constexpr char src[] =\
	"#version 430\n\
	\n\
	void main()\n\
	{\n\
	\n\
	}";

constexpr char src2[] =\
	"#version 430\n\
	\n\
	#include \"include_me.header.glsl\"\n\
	void main()\n\
	{\n\
	\n\
	}";

constexpr char src3[] =\
	"#version 430\n\
	\n\
	#include \"include_me.header.glsl\"\n\
	#include \"include_me2.header.glsl\"\n\
	\n\
	#include \"include_me3.header.glsl\"\n\
	void main()\n\
	{\n\
	\n\
	}";

namespace tz::test
{
	class TestUppercaseModule : public tz::gl::p::IModule
	{
		virtual void operator()(std::string& source) const override
		{
			for(char& c : source)
				c = std::toupper(c);
		}
	};

	class TestLowercaseModule : public tz::gl::p::IModule
	{
		virtual void operator()(std::string& source) const override
		{
			for(char& c : source)
				c = std::tolower(c);
		}
	};
}

tz::test::Case no_modules()
{
	tz::test::Case test_case("tz::gl::ShaderPreprocessor Empty Tests");
	tz::gl::ShaderPreprocessor pre{src};
	topaz_expect(test_case, pre.empty(), "tz::gl::ShaderPreprocessor constructed with mere source failed to be empty! Size: ", pre.size());
	pre.preprocess();
	topaz_expect_assert(test_case, false, "tz::gl::ShaderPreprocessor::preprocess(): Asserted even though we don't even have any modules!");
	topaz_expect(test_case, pre.result() == src, "tz::gl::ShaderPreprocessor::preprocess(): Made changes to the source even though there aren't any modules! Before: \n\"", src, "\"\nAfter:\"", pre.result(), "\"");
	return test_case;
}

tz::test::Case example_module()
{
	tz::test::Case test_case("tz::gl::ShaderPreprocessor Example Module Tests (Uppercase TZGLP)");
	tz::gl::ShaderPreprocessor pre{src};
	pre.emplace_module<tz::test::TestUppercaseModule>();
	pre.preprocess();
	// All chars that can be upper-case should be upper.
	for(char c : pre.result())
	{
		topaz_expect(test_case, !std::islower(c), "tz::gl::p Module TestUppercase Failed: Found non-uppercase character '", c, "'");
	}
	return test_case;
}

tz::test::Case module_order()
{
	tz::test::Case test_case("tz::gl::ShaderPreprocessor Module Ordering Tests");
	tz::gl::ShaderPreprocessor pre{src};
	std::size_t u = pre.emplace_module<tz::test::TestUppercaseModule>();
	std::size_t l = pre.emplace_module<tz::test::TestLowercaseModule>();
	pre.preprocess();
	// All chars that can be upper-case should be lower. All alphabetic chars should be uppered by the 0th module, but lowered again by the 1st module.
	// Module order must matter!
	for(char c : pre.result())
	{
		topaz_expect(test_case, !std::isupper(c), "tz::gl::p Module Ordering Failed: Found non-lowercase character '", c, "'");
	}
	// Now swap modules!
	pre.swap_modules(u, l);
	pre.preprocess();
	// This time, the uppercase happens last, so everything should be upper now!
	for(char c : pre.result())
	{
		topaz_expect(test_case, !std::islower(c), "tz::gl::p Module Ordering Failed: Found non-uppercase character '", c, "'");
	}
	return test_case;
}

tz::test::Case include_file()
{
	tz::test::Case test_case("tz::gl::p IncludeModule Tests");
	// Easy test-case.
	{
		tz::gl::ShaderPreprocessor pre{src2};
		pre.emplace_module<tz::gl::p::IncludeModule>(__FILE__);
		// src2 includes "include_me.header.glsl" which is guaranteed to be in the same directory as this source file.
		// it is also guaranteed to contain the string "Plums"
		// if the preprocessed source successfully contains the string "Plums", then we will know the include worked.
		pre.preprocess();
		std::size_t find_result = pre.result().find("Plums");
		topaz_expect(test_case, find_result != std::string::npos, "tz::gl::p::IncludeModule failed to process include correctly. Preprocessed source: \n\"", pre.result(), "\"");
	}
	// Might have got lucky with a singular include, let's try three!
	{
		// src3 includes "include_me2" and "include_me3", which respectively contain the strings "Apples" and "Oranges".
		tz::gl::ShaderPreprocessor pre{src3};
		pre.emplace_module<tz::gl::p::IncludeModule>(__FILE__);
		pre.preprocess();
		std::size_t a = pre.result().find("Plums");
		std::size_t b = pre.result().find("Apples");
		std::size_t c = pre.result().find("Oranges");
		topaz_expect(test_case, a != std::string::npos, "tz::gl::p::IncludeModule failed to process include correctly. Preprocessed source: \n\"", pre.result(), "\"");
		topaz_expect(test_case, b != std::string::npos, "tz::gl::p::IncludeModule failed to process include correctly. Preprocessed source: \n\"", pre.result(), "\"");
		topaz_expect(test_case, c != std::string::npos, "tz::gl::p::IncludeModule failed to process include correctly. Preprocessed source: \n\"", pre.result(), "\"");
	}
	return test_case;
}

int main()
{
    tz::test::Unit pre;

    // We require topaz to be initialised.
    {
        tz::core::initialise("Shader Preprocessor Tests");
		pre.add(no_modules());
		pre.add(example_module());
		pre.add(module_order());
		pre.add(include_file());
        tz::core::terminate();
    }
    return pre.result();
}