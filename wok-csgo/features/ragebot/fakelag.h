#include "../../globals.h"

class c_fakelag : public c_singleton<c_fakelag> {
public:

	void on_create_move();
};

#define fakelag c_fakelag::instance()