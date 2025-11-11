#ifndef __USER_H__
#define __USER_H__

#include <string>

// User表的ORM类
class User {
public:
    User(int id = -1, std::string name = "", std::string pwd = "",
         std::string state = "offline") {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    // Getters
    int getId() const { return this->id; }
    const std::string &getName() const { return this->name; }
    const std::string &getPassword() const { return this->password; }
    const std::string &getState() const { return this->state; }

    // Setters
    void setId(int id) { this->id = id; }
    void setName(const std::string &name) { this->name = name; }
    void setPassword(const std::string &password) { this->password = password; }
    void setState(const std::string &state) { this->state = state; }

private:
    int id;
    std::string name;
    std::string password;
    std::string state;
};

#endif // __USER_H__