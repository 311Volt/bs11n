#include <bs11n.hpp>
#include <map>
#include <iostream>

#include <bsoncxx/json.hpp>

struct Address {
    std::string street;
	int32_t number;

	BS11N_DOCUMENT(street, number)
};

struct Consents {
	bool emailContact = false;
	bool phoneContact = false;

	BS11N_DOCUMENT(emailContact, phoneContact)
};

struct User {
	std::string firstName;
	std::string lastName;
	std::optional<bsoncxx::types::b_date> dateRegistered;
	std::vector<int> favoriteNumbers;
	std::map<std::string, Address> addresses;
	Consents consents;
	std::vector<uint8_t> bin;

	BS11N_DOCUMENT(firstName, lastName, dateRegistered, favoriteNumbers, addresses, consents, bin)
};

int main()
{
	User user = {
		.firstName = "Jan",
		.lastName = "Kowalski",
		.favoriteNumbers = {1, 4, 3, 7},
		.addresses = {
			{"main", Address{.street = "example1", .number = 7}},
			{"other", Address{.street = "example2", .number = 17}}
		},
		.bin = {1,2,3,4,5,6,7}
	};

	bsoncxx::document::value serialized = user.bs11nDump();
	User restoredUser = bs11n::Deserialize<User>(serialized);
	std::cout << bsoncxx::to_json(restoredUser.bs11nDump()) << "\n";
}