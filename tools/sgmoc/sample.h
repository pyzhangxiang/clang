#include "sgMetaDef.h"

void Func1();
SG_META_NO_EXPORT void Func2();

enum SG_META_ENUM RoomType
{
    
};

struct Person 
{
};


class Room : public XX
{
    SG_META_OBJECT(Room)
public:
    SG_META_NO_EXPORT int aa;
    float ff;
    const char *cc;
    bool bb2;
    int arr[12];
    
    SG_META_NO_EXPORT void add_person(Person person)
    {
        
    }
    

    struct ABC{};
    ABC abc;

    enum EnumAA
    {

    };

private:
    Person* people_in_room;
};



