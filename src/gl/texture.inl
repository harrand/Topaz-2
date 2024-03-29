namespace tz::gl
{
	template<template<typename> class PixelType, typename ComponentType>
	void Texture::set_data(const tz::gl::Image<PixelType<ComponentType>>& image)
	{
		constexpr GLint internal_format = tz::gl::pixel::parse_internal_format<PixelType, ComponentType>();
		constexpr GLenum format = tz::gl::pixel::parse_format<PixelType, ComponentType>();
		constexpr GLenum type = tz::gl::pixel::parse_component_type<ComponentType>();
		static_assert(internal_format != GL_INVALID_VALUE, "Texture::set_data<PixelType, ComponentType>: Unsupported pixel/component types.");
		static_assert(format != GL_INVALID_VALUE, "Texture::set_data<PixelType, ComponentType>: Unsupported pixel/component types.");
		static_assert(type != GL_INVALID_VALUE, "Texture::set_data<PixelType, ComponentType>: Unsupported pixel/component types.");
		this->internal_bind();
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, static_cast<GLsizei>(image.get_width()), static_cast<GLsizei>(image.get_height()), 0, format, type, image.data());
		this->descriptor = {type, internal_format, format, image.get_width(), image.get_height()};
	}

	template<template<typename> class PixelType, typename ComponentType>
	tz::gl::Image<PixelType<ComponentType>> Texture::get_data() const
	{
		topaz_assert(this->descriptor.has_value(), "tz::gl::Texture::get_data<PixelType, ComponentType>(): Cannot get data because we never detected an invocation of set_data!");
		constexpr GLint internal_format = tz::gl::pixel::parse_internal_format<PixelType, ComponentType>();
		constexpr GLenum format = tz::gl::pixel::parse_format<PixelType, ComponentType>();
		constexpr GLenum type = tz::gl::pixel::parse_component_type<ComponentType>();
		TextureDataDescriptor desired_descriptor{type, internal_format, format};
		TextureDataDescriptor expected_descriptor = this->descriptor.value();
		topaz_assert(desired_descriptor == expected_descriptor, "tz::gl::Texture::get_data<PixelType, ComponentType>(): TextureDataDescriptors didn't match. This means you are attempting to retrieve a texture with a different descriptor than what was set:\n\
		Desired Descriptor:\n\
		\tComponent-Type: ", type, "\n\
		\tInternal-Format: ", internal_format, "\n\
		\tFormat: ", format, "\
		Provided Descriptor:\n\
		\tComponent-Type: ", expected_descriptor.component_type, "\n\
		\tInternal-Format: ", expected_descriptor.internal_format, "\n\
		\tFormat: ", expected_descriptor.format);

		// Make the empty image.
		tz::gl::Image<PixelType<ComponentType>> img{expected_descriptor.width, expected_descriptor.height};
		// Can safely get the image data now. img.data() will have the correct allocation size.
		this->internal_bind();
		glGetTexImage(GL_TEXTURE_2D, 0, format, type, img.data());
		return std::move(img);
	}
}