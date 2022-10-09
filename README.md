# bs11n

A quick and dirty C++20 header-only library for BSON serialization and deserialization.

This library was made with Crow web services in mind. Since BSON is a relatively compact format
that is easily converted to and from JSON, one can use the same serialization code for MongoDB
persistence, message queues and HTTP requests.

# usage

Include `bs11n.hpp` in any project with [mongo-cxx-driver](https://github.com/mongodb/mongo-cxx-driver) set up.

# example

```c++
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

    BS11N_DOCUMENT(firstName, lastName, dateRegistered, favoriteNumbers, addresses, consents)
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
        }
    };

    bsoncxx::document::value serialized = user.bs11nDump();
    User restoredUser = bs11n::Deserialize<User>(serialized);
    std::cout << bsoncxx::to_json(restoredUser.bs11nDump()) << "\n";
}
```

## Output (formatted)

```json
{
    "firstName": "Jan",
    "lastName": "Kowalski",
    "dateRegistered": null,
    "favoriteNumbers": [1,4,3,7],
    "addresses": {
        "main": {
            "street": "example1",
            "number": 7
        },
        "other": {
            "street": "example2",
            "number": 17
        }
    },
    "consents": {
        "emailContact": false,
        "phoneContact": false
    }
}
```