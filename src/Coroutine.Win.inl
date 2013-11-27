/*
 * Copyright (c) 2013 Matt Fichman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, APEXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */


namespace coro {

LONG WINAPI fault(LPEXCEPTION_POINTERS info) {
    // From MSDN: The first element of the array contains a read-write flag
    // that indicates the type of operation that caused the access violation.
    // If this value is zero, the thread attempted to read the inaccessible
    // data. If this value is 1, the thread attempted to write to an
    // inaccessible address. If this value is 8, the thread causes a user-mode
    // data execution prevention (DEP) violation.  The second array element
    // specifies the virtual address of the inaccessible data.
    printf("alloc\n");
    uint64_t stackStart = (uint64_t)coroCurrent->stack_.begin();
    uint64_t stackEnd = (uint64_t)coroCurrent->stack_.end();
    uint64_t type = (uint64_t)info->ExceptionRecord->ExceptionInformation[0];
    uint64_t addr = (uint64_t)info->ExceptionRecord->ExceptionInformation[1];
	DWORD code = info->ExceptionRecord->ExceptionCode;
    if (type != 0 && type != 1) {
        return EXCEPTION_CONTINUE_SEARCH;
    } else if (coroCurrent == main().get()) {
        return EXCEPTION_CONTINUE_SEARCH;
    } else if (addr < stackStart || addr >= stackEnd) {
        return EXCEPTION_CONTINUE_SEARCH;
    } else if (code == EXCEPTION_ACCESS_VIOLATION) {
        coroCurrent->commit(addr);
        return EXCEPTION_CONTINUE_EXECUTION;
    } else if (code == STATUS_GUARD_PAGE_VIOLATION) {
        coroCurrent->commit(addr);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void registerSignalHandlers() {
    // Set up the signal handlers for SIGBUS/SIGSEGV to catch stack protection
    AddVectoredExceptionHandler(1, fault);
}

}
