#ifndef INCLUDE_BS11N
#define INCLUDE_BS11N

#include <bsoncxx/types/bson_value/value.hpp>
#include <bsoncxx/types/bson_value/view_or_value.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>


#include <string>
#include <vector>
#include <span>
#include <stdexcept>
#include <concepts>


#define BS11N_PARENS ()

#define BS11N_EXPAND(...) BS11N_EXPAND4(BS11N_EXPAND4(BS11N_EXPAND4(BS11N_EXPAND4(__VA_ARGS__))))
#define BS11N_EXPAND4(...) BS11N_EXPAND3(BS11N_EXPAND3(BS11N_EXPAND3(BS11N_EXPAND3(__VA_ARGS__))))
#define BS11N_EXPAND3(...) BS11N_EXPAND2(BS11N_EXPAND2(BS11N_EXPAND2(BS11N_EXPAND2(__VA_ARGS__))))
#define BS11N_EXPAND2(...) BS11N_EXPAND1(BS11N_EXPAND1(BS11N_EXPAND1(BS11N_EXPAND1(__VA_ARGS__))))
#define BS11N_EXPAND1(...) __VA_ARGS__

#define BS11N_FOR_EACH(macro, ...)                                    \
	__VA_OPT__(BS11N_EXPAND(BS11N_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define BS11N_FOR_EACH_HELPER(macro, a1, ...)                         \
	macro(a1)                                                     \
	__VA_OPT__(BS11N_FOR_EACH_AGAIN BS11N_PARENS (macro, __VA_ARGS__))
#define BS11N_FOR_EACH_AGAIN() BS11N_FOR_EACH_HELPER

#define BS11N_DOCUMENT_SERIALIZE_FIELD(fieldName) \
	builder.append(bld::kvp(#fieldName, ::bs11n::Serialize<decltype(this->fieldName)>(this->fieldName)));

#define BS11N_DOCUMENT_DUMP_METHOD(...) \
	inline bsoncxx::document::value bs11nDump() const { \
		namespace bld = bsoncxx::builder::basic; \
		bld::document builder; \
		BS11N_FOR_EACH(BS11N_DOCUMENT_SERIALIZE_FIELD, __VA_ARGS__) \
		return builder.extract(); \
	}

#define BS11N_DOCUMENT_DESERIALIZE_FIELD(fieldName) \
	this->fieldName = ::bs11n::Deserialize<decltype(this->fieldName)>(doc.find(#fieldName)->get_value());

#define BS11N_DOCUMENT_INIT_METHOD(...) \
	inline void bs11nInit(bsoncxx::types::bson_value::view val) { \
		::bs11n::CheckType<bsoncxx::type::k_document>(val.type()); \
		bsoncxx::document::view doc = val.get_document().value; \
		BS11N_FOR_EACH(BS11N_DOCUMENT_DESERIALIZE_FIELD, __VA_ARGS__) \
	} 


#define BS11N_DOCUMENT(...) \
	BS11N_DOCUMENT_DUMP_METHOD(__VA_ARGS__) \
	BS11N_DOCUMENT_INIT_METHOD(__VA_ARGS__) 


namespace bs11n {

	struct DeserializeException: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	struct TypeMismatchException: public DeserializeException {
		using DeserializeException::DeserializeException;
		inline TypeMismatchException(bsoncxx::type expected, bsoncxx::type actual)
			: TypeMismatchException("tried to deserialize " +
				bsoncxx::to_string(actual) + " into " + 
				bsoncxx::to_string(expected))
		{}
	};

	template<typename T>
	concept TriviallySerializable = requires(T) {
		requires std::same_as<typename std::remove_cv<decltype(T::type_id)>::type, bsoncxx::type>;
	};

	template<typename T>
	concept BasicSerializable = 
		!TriviallySerializable<T> && 
		std::convertible_to<T, bsoncxx::types::bson_value::value>;

	template<typename T>
	concept DocumentClass = requires(T val) {
		{val.bs11nDump()} -> std::same_as<bsoncxx::document::value>;
		val.bs11nInit(bsoncxx::types::bson_value::view{});
	};

	template<typename T>
	concept Serializable = TriviallySerializable<T> || BasicSerializable<T> || DocumentClass<T>;

	template<typename T>
	concept SerializableMap = requires(T map) {
		requires std::convertible_to<typename T::key_type, std::string>;
		requires Serializable<typename T::mapped_type>;
		{map.begin()} -> std::same_as<typename T::iterator>;
		{map.end()} -> std::same_as<typename T::iterator>;
	};

	template<typename T>
	concept SerializableList = requires(T list) {
		requires Serializable<typename T::value_type>;
		{list.begin()} -> std::same_as<typename T::iterator>;
		{list.end()} -> std::same_as<typename T::iterator>;
		list.push_back(typename T::value_type{});
		requires !std::same_as<T, std::basic_string<typename T::value_type>>;
		requires !std::same_as<T, std::vector<uint8_t>>;
	};

	template<typename T>
	concept SerializableOptional = requires(T) {
		requires std::convertible_to<std::nullopt_t, T>;
		requires Serializable<typename T::value_type>;
	};

	template<bsoncxx::type expected>
	void CheckType(bsoncxx::type actual)
	{
		if (expected != actual) {
			throw TypeMismatchException(expected, actual);
		}
	}


	template<TriviallySerializable T>
	bsoncxx::types::bson_value::value Serialize(const T& value)
	{
		return value;
	}

	template<TriviallySerializable T>
	T Deserialize(bsoncxx::types::bson_value::view val)
	{
	#define BSONCXX_ENUM(typeName, typenum) \
		if constexpr(bsoncxx::type{typenum} == T::type_id) { \
			CheckType<T::type_id>(val.type()); \
			return val.get_##typeName(); \
		} 

		#include <bsoncxx/enums/type.hpp>

	#undef BSONCXX_ENUM

		throw DeserializeException("wtf");
	}

	template<DocumentClass T>
	bsoncxx::types::bson_value::value Serialize(const T& value)
	{
		return bsoncxx::types::bson_value::value {value.bs11nDump()};
	}

	template<DocumentClass T>
	T Deserialize(bsoncxx::types::bson_value::view val)
	{
		T ret;
		ret.bs11nInit(val);
		return ret;
	}

	template<DocumentClass T>
	T Deserialize(bsoncxx::document::view val)
	{
		return Deserialize<T>(bsoncxx::types::bson_value::value{val});
	}


	template<BasicSerializable T>
	bsoncxx::types::bson_value::value Serialize(T value)
	{
		static_assert(!std::is_same_v<T, T>, "type not serializable");
		return {nullptr};
	}

	template<BasicSerializable T>
	T Deserialize(bsoncxx::types::bson_value::view val)
	{
		static_assert(!std::is_same_v<T, T>, "type not serializable");
		return {};
	}

	struct OIDTransferStruct {
		bsoncxx::oid oid;
		inline OIDTransferStruct(bsoncxx::types::b_oid oid) : oid(oid.value) {}
		inline operator bsoncxx::oid() {return oid;}
	};

	struct BinaryTransferStruct {
		std::span<uint8_t> data;
		inline BinaryTransferStruct(bsoncxx::types::b_binary bin) 
			: data((decltype(data)::pointer)bin.bytes, (decltype(data)::size_type)bin.size) 
		{}
		inline operator std::vector<uint8_t>()
		{
			return std::vector<uint8_t>(data.begin(), data.end());
		}
	};

#define BS11N_BASIC_SERIALIZABLE(typeName, conversionType, bsonType) \
	template<> \
	inline bsoncxx::types::bson_value::value Serialize<typeName>(typeName value) \
		{return value;} \
	template<> \
	inline typeName Deserialize<typeName>(bsoncxx::types::bson_value::view val) \
		{CheckType<bsoncxx::type::k_##bsonType>(val.type()); return conversionType{val.get_##bsonType()};} \


	BS11N_BASIC_SERIALIZABLE(int32_t, int32_t, int32)
	BS11N_BASIC_SERIALIZABLE(int64_t, int64_t, int64)
	BS11N_BASIC_SERIALIZABLE(bool, bool, bool)
	BS11N_BASIC_SERIALIZABLE(float, double, double)
	BS11N_BASIC_SERIALIZABLE(double, double, double)
	
	BS11N_BASIC_SERIALIZABLE(std::string, std::string, string)
	BS11N_BASIC_SERIALIZABLE(bsoncxx::oid, OIDTransferStruct, oid)
	BS11N_BASIC_SERIALIZABLE(std::vector<uint8_t>, BinaryTransferStruct, binary)
	

#undef BS11N_BASIC_SERIALIZABLE

	template<SerializableMap T>
	bsoncxx::types::bson_value::value Serialize(const T& map)
	{
		namespace bld = bsoncxx::builder::basic;
		bld::document builder;

		for(const auto& [key, value]: map) {
			builder.append(bld::kvp(key, Serialize<typename T::mapped_type>(value)));
		}

		return builder.extract().view();
	}

	template<SerializableMap T>
	T Deserialize(bsoncxx::types::bson_value::view val)
	{
		T ret;
		for(const auto& element: val.get_document().value) {
			ret.insert({std::string{element.key()}, Deserialize<typename T::mapped_type>(element.get_value())});
		}
		return ret;
	}

	template<SerializableList T>
	bsoncxx::types::bson_value::value Serialize(const T& list)
	{
		namespace bld = bsoncxx::builder::basic;
		bld::array builder;
		for(const auto& value: list) {
			builder.append(Serialize<typename T::value_type>(value));
		}

		return builder.extract().view();
	}

	template<SerializableList T>
	T Deserialize(bsoncxx::types::bson_value::view val)
	{
		T ret;
		for(const auto& value: val.get_array().value) {
			ret.push_back(Deserialize<typename T::value_type>(value.get_value()));
		}
		return ret;
	}

	template<SerializableOptional T>
	bsoncxx::types::bson_value::value Serialize(const T& value)
	{
		if(value.has_value()) {
			return Serialize<typename T::value_type>(*value);
		}
		return bsoncxx::types::b_null {};
	}

	template<SerializableOptional T>
	T Deserialize(bsoncxx::types::bson_value::view val)
	{
		if(val.type() != bsoncxx::type::k_null) {
			return Deserialize<typename T::value_type>(val);
		}
		return std::nullopt;
	}


}


#endif /* INCLUDE_BS11N */
