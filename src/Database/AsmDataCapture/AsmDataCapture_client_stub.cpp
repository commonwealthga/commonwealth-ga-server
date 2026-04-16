// Client-only stub for AsmDataCapture. The real implementation lives in
// AsmDataCapture.cpp, which pulls in Database/sqlite3 — unneeded on the
// client side. CMarshal__GetArray (linked into both builds) references
// AsmDataCapture::OnGetArray and the bPopulateDatabase flag, so we provide
// empty symbols for the client target to satisfy the linker.
#include "src/Database/AsmDataCapture/AsmDataCapture.hpp"

namespace AsmDataCapture {
	bool bPopulateDatabase = false;

	void OnGetArray(void* /*marshal*/, int /*field*/, uint32_t /*arrayPtr*/) {
		// no-op on client
	}
}
