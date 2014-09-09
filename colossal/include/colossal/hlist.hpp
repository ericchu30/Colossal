/* Colossal
 * Copyright (c) 2014 Zilong Tan (eric.zltan@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, subject to the conditions listed
 * in the Colossal LICENSE file. These conditions include: you must preserve this
 * copyright notice, and you cannot mention the copyright holders in
 * advertising related to the Software without their permission.  The Software
 * is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This notice is a
 * summary of the Colossal LICENSE file; the license in that file is legally
 * binding.
 */

#ifndef _COLOSSAL_HLIST_H
#define _COLOSSAL_HLIST_H

#include <ulib/hash_chain_prot.h>
#include "common.hpp"

namespace colossal
{

template<class _Key, class _Except = except>
class hlist
{
public:
        DEFINE_CHAINHASH(inclass, _Key, int, 0, chainhash_hashfn, chainhash_equalfn, chainhash_cmpfn);

        typedef _Key        key_type;
        typedef size_t    size_type;
        typedef chainhash_t(inclass)   * hashing;
        typedef chainhash_itr_t(inclass) hashing_iterator;

        struct iterator {
                typedef size_t size_type;
                typedef chainhash_itr_t(inclass) hashing_iterator;

                chainhash_itr_t(inclass) _cur;

                iterator(hashing_iterator itr)
                        : _cur(itr) { }

                iterator() { }

                _Key &
                key() const
                {
                        return chainhash_key(_cur);
                }

                bool
                value() const
                {
                        return !chainhash_end(_cur);
                }

                bool
                operator*() const
                {
                        return value();
                }

                iterator&
                operator++()
                {
                        if (chainhash_advance(inclass, &_cur))
                                _cur.entry = NULL;
                        return *this;
                }

                iterator
                operator++(int)
                {
                        iterator old = *this;
                        ++*this;
                        return old;
                }

                bool
                operator==(const iterator &other) const
                {
                        return _cur.entry == other._cur.entry;
                }

                bool
                operator!=(const iterator &other) const
                {
                        return _cur.entry != other._cur.entry;
                }
        };

        struct const_iterator {
                typedef size_t size_type;
                typedef const chainhash_itr_t(inclass) hashing_iterator;

                chainhash_itr_t(inclass) _cur;

                const_iterator(hashing_iterator itr)
                        : _cur(itr) { }

                const_iterator() { }

                const_iterator(const iterator &it)
                        : _cur(it._cur) { }

                const _Key &
                key() const
                {
                        return chainhash_key(_cur);
                }

                bool
                value() const
                {
                        return !chainhash_end(_cur);
                }

                bool
                operator*() const
                {
                        return value();
                }

                const_iterator &
                operator++()
                {
                        if (chainhash_advance(inclass, &_cur))
                                _cur.entry = NULL;
                        return *this;
                }

                const_iterator
                operator++(int)
                {
                        const_iterator old = *this;
                        ++*this;
                        return old;
                }

                bool
                operator==(const const_iterator &other) const
                {
                        return _cur.entry == other._cur.entry;
                }

                bool
                operator!=(const const_iterator &other) const
                {
                        return _cur.entry != other._cur.entry;
                }
        };

        hlist(size_t min)
        {
                _hashing = chainhash_init(inclass, min);
                if (_hashing == 0)
                        throw _Except();
        }

        hlist(const hlist &other)
        {
                _hashing = chainhash_init(inclass, other.bucket_count());
                if (_hashing == 0)
                        throw _Except();
                for (const_iterator it = other.begin(); it != other.end(); ++it)
                        insert(it.key());
        }

        hlist &
        operator= (const hlist &other)
        {
                if (&other != this) {
                        clear();
                        for (const_iterator it = other.begin(); it != other.end(); ++it)
                                insert(it.key());
                }
                return *this;
        }

        virtual
        ~hlist()
        {
                chainhash_destroy(inclass, _hashing);
        }

        iterator
        begin()
        {
                return iterator(chainhash_begin(inclass, _hashing));
        }

        iterator
        end()
        {
                chainhash_itr_t(inclass) itr;
                itr.entry = NULL;
                return iterator(itr);
        }

        const_iterator
        begin() const
        {
                return const_iterator(chainhash_begin(inclass, _hashing));
        }

        const_iterator
        end() const
        {
                chainhash_itr_t(inclass) itr;
                itr.entry = NULL;
                return const_iterator(itr);
        }

        bool
        contain(const _Key &key) const
        {
                return chainhash_get(inclass, _hashing, key).entry != NULL;
        }

        iterator
        insert(const _Key &key)
        {
                hashing_iterator itr = chainhash_set(inclass, _hashing, key);
                if (itr.entry == NULL)
                        throw _Except();
                return iterator(itr);
        }

        bool
        operator[](const _Key &key)
        {
                return contain(key);
        }

        iterator
        find(const _Key &key)
        {
                return iterator(chainhash_get(inclass, _hashing, key));
        }

        const_iterator
        find(const _Key &key) const
        {
                return const_iterator(chainhash_get(inclass, _hashing, key));
        }

        void
        erase(const _Key &key)
        {
                chainhash_del(inclass, chainhash_get(inclass, _hashing, key));
        }

        void
        erase(const iterator &it)
        {
                chainhash_del(inclass, it._cur);
        }

        void
        clear()
        {
                chainhash_clear(inclass, _hashing);
        }

        void
        snap()
        {
                chainhash_snap(inclass, _hashing);
        }

        void
        sort()
        {
                chainhash_sort(inclass, _hashing);
        }

        size_t
        size() const
        {
                size_t n = 0;
                for (const_iterator it = begin(); it != end(); ++it)
                        ++n;
                return n;
        }

        size_t
        bucket_count() const
        {
                return chainhash_nbucket(_hashing);
        }

private:
        hashing _hashing;
};

}

#endif
