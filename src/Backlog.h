
/*
 * Copyright (c) 2011 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

template<class P, class T> class Backlog
{
protected:

    unsigned int Count;
    vector<T *> List;

    Lacewing::SpinSync Sync;

public:

    inline Backlog()
    {
        Count = 8;
    }

    inline Backlog(unsigned int Count)
    {
        this->Count = Count;   
    }

    inline ~Backlog()
    {
        for(typename vector<T *>::iterator it = List.begin(); it != List.end(); ++ it)
        {
            #ifdef LacewingWindows
            #ifdef LacewingDebug

                DWORD Old;

                VirtualProtect(*it, sizeof(T), PAGE_READWRITE, &Old);
                VirtualFree(*it, MEM_RELEASE, 0);

                continue;

            #endif
            #endif

            free(*it);
        }
    }

    inline T &Borrow(P &Parent)
    {
        T * Borrowed;

        {   Lacewing::SpinSync::WriteLock Lock(Sync);

            if(!List.size())
            {
                for(unsigned int i = Count; i; -- i)
                {
                    #ifdef LacewingDebug
                    #ifdef LacewingWindows
        
                        List.push_back((T *) VirtualAlloc(0, sizeof(T), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
                        continue;

                    #endif
                    #endif
            
                    List.push_back((T *) malloc(sizeof(T)));
                }
            }

            Borrowed = List.back();
            List.pop_back();
        }

        #ifdef LacewingDebug
        #ifdef LacewingWindows

            /* Restore normal protection (see Return) */

            DWORD Old;
            VirtualProtect(Borrowed, sizeof(T), PAGE_READWRITE, &Old);

        #endif
        #endif

        return * new (Borrowed) T (Parent);
    }

    inline void Return(T &Object)
    {
        Object.~T();

        #ifdef LacewingDebug

            memset(&Object, 0, sizeof(T));

            #ifdef LacewingWindows

                /* Protect the memory so that an exception is thrown if something is used after
                   being returned to the backlog */

                DWORD Old;
                VirtualProtect(&Object, sizeof(T), PAGE_NOACCESS, &Old);

            #endif

        #endif

        Lacewing::SpinSync::WriteLock Lock(Sync);
        List.push_back(&Object);
    }


};

