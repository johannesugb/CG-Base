namespace cgb
{
	input_description input_description::create(std::initializer_list<input_binding_location_data> pBindings)
	{
		input_description result;

		for (const auto& bindingLoc : pBindings) {
			auto& bfr = result.mInputBuffers[bindingLoc.mBinding];
			// Create if it doesn't exist
			if (std::holds_alternative<std::monostate>(bfr)) {
				switch (bindingLoc.mKind)
				{
				case input_binding_location_data::kind::vertex:
					bfr = vertex_buffer_meta::create_from_element_size(bindingLoc.mStride);
					break;
				case input_binding_location_data::kind::instance:
					bfr = instance_buffer_meta::create_from_element_size(bindingLoc.mStride);
					break;
				default:
					throw std::runtime_error("Invalid input_binding_location_data::kind value");
				}
			}

#if defined(_DEBUG)
			// It exists => perform some sanity checks:
			if (std::holds_alternative<std::monostate>(bfr)
				|| (input_binding_location_data::kind::vertex == bindingLoc.mKind && std::holds_alternative<instance_buffer_meta>(bfr))
				|| (input_binding_location_data::kind::instance == bindingLoc.mKind && std::holds_alternative<vertex_buffer_meta>(bfr))
				) {
				throw std::logic_error("All locations of the same binding must come from the same buffer type (vertex buffer or instance buffer).");
			}
#endif

			if (std::holds_alternative<vertex_buffer_meta>(bfr)) {
				std::get<vertex_buffer_meta>(bfr).describe_member_location(
					bindingLoc.mLocation.mLocation,
					bindingLoc.mLocation.mOffset,
					bindingLoc.mLocation.mFormat);
			}
			else { // must be instance_buffer_meta
				std::get<instance_buffer_meta>(bfr).describe_member_location(
					bindingLoc.mLocation.mLocation,
					bindingLoc.mLocation.mOffset,
					bindingLoc.mLocation.mFormat);
			}
		}

		return result;
	}
}