#ifndef BOOST_TRIE_NODE_HPP
#define BOOST_TRIE_NODE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <map>
#include <vector>
#include <boost/utility.hpp>
#include <boost/intrusive/set.hpp>
#include <memory>

namespace boost { namespace tries {

namespace detail {

template <typename Key, typename Value>
struct trie_node;

struct list_node_base : protected boost::noncopyable {
	typedef list_node_base *base_ptr;
	base_ptr pred;
	base_ptr next;

	list_node_base() : pred(0), next(0)
	{}
};

template <typename Key, typename Value>
struct value_list_node : public list_node_base {
	typedef Key key_type;
	typedef Value value_type;
	typedef trie_node<key_type, value_type> trie_node_type;
	typedef trie_node_type * trie_node_ptr;
	typedef value_list_node<key_type, value_type> node_type;
	typedef node_type * node_ptr;
	typedef list_node_base * base_ptr;
	value_type value;
	trie_node_ptr node_in_trie;

	explicit value_list_node() : value(), node_in_trie(0)
	{
	}

	explicit value_list_node(const value_type& x) : value(x), node_in_trie(0)
	{
	}

	~value_list_node() {
		node_in_trie->self_value_count--;
	}
};

template <typename Key, typename Value>
struct trie_node : private boost::noncopyable,
				   public boost::intrusive::set_base_hook<>
{
	typedef Key key_type;
	typedef Value value_type;
	typedef value_type * value_ptr;
	typedef size_t size_type;
	typedef trie_node<key_type, value_type> node_type;
	typedef node_type* node_ptr;
	typedef value_list_node<key_type, value_type> value_list_type;
	typedef value_list_type * value_list_ptr;
	typedef boost::intrusive::set<node_type> children_type;
	typedef typename children_type::iterator children_iter;

	key_type key;
	children_type children;
	node_ptr parent;
	// store the iterator to optimize operator++ and operator--
	// utilize that the iterator in map does not change after insertion
	size_type value_count;
	size_type self_value_count;
	value_list_ptr value_list_header;
	value_list_ptr value_list_tail;

	node_ptr pred_node;
	node_ptr next_node;

	explicit trie_node() : parent(0), value_count(0), self_value_count(0),
	value_list_header(0), value_list_tail(0),
	pred_node(0), next_node(0)
	{
	}

	explicit trie_node(const key_type& key) : key(key), parent(0), value_count(0), self_value_count(0),
	value_list_header(0), value_list_tail(0),
	pred_node(0), next_node(0)
	{
	}

	const key_type& key_elem() const
	{
		return key;
	}

	size_type count() const
	{
		return self_value_count;
	}

	bool no_value() const
	{
		return self_value_count == 0;
	}

	template<typename Allocator>
	void remove_values(Allocator& alloc) {
		value_list_ptr vp = value_list_header;
		if (!no_value())
		{
			while (vp != NULL)
			{
				value_list_ptr tmp = static_cast<value_list_ptr>(vp->next);
				alloc.destroy(vp);
				alloc.deallocate(vp, 1);
				vp = tmp;
			}
		}
		self_value_count = 0;
		value_list_header = value_list_tail = NULL;
	}

	template<typename Allocator>
	void add_value(const value_type& value, Allocator& alloc) {
		value_list_ptr vn = alloc.allocate(1);
		vn = new(vn) value_list_type(value);
		vn->node_in_trie = this;
		vn->next = this->value_list_header;
		if (this->value_list_header != NULL)
		{
			this->value_list_header->pred = vn;
		}
		else {
			this->value_list_tail = vn;
		}
		this->value_list_header = vn;
		++this->self_value_count;
	}

	template<typename Allocator>
	void copy_values_from(const node_type& other, Allocator& alloc) {
		value_list_ptr vp = other.value_list_header;
		while (vp != NULL) {
			this->add_value(vp->value, alloc);
			vp = static_cast<value_list_ptr>(vp->next);
		}
		self_value_count = other.self_value_count;
		value_count = other.value_count;
	}

	friend bool operator < (const node_type& a, const node_type& b) {
		return a.key < b.key;
	}
	friend bool operator > (const node_type& a, const node_type& b) {
		return a.key > b.key;
	}
	friend bool operator == (const node_type& a, const node_type& b) {
		return a.key == b.key;
	}
};

template <typename Key>
struct trie_node<Key, void> : private boost::noncopyable,
							  public boost::intrusive::set_base_hook<>
{
	typedef Key key_type;
	typedef void value_type;
	typedef value_type * value_ptr;
	typedef size_t size_type;
	typedef trie_node<key_type, value_type> node_type;
	typedef node_type* node_ptr;
	typedef boost::intrusive::set<node_type> children_type;
	typedef typename children_type::iterator children_iter;

	key_type key;
	children_type children;
	node_ptr parent;
	size_type value_count;
	bool key_ends_here;
	node_ptr pred_node;
	node_ptr next_node;

	explicit trie_node() : parent(0), value_count(0),  key_ends_here(false), pred_node(0), next_node(0)
	{
	}

	explicit trie_node(const key_type& key) : key(key), parent(0), value_count(0),  key_ends_here(false), pred_node(0), next_node(0)
	{
	}

	const key_type& key_elem() const
	{
		return key;
	}

	size_type count() const
	{
		return key_ends_here;
	}

	bool no_value() const
	{
		return !key_ends_here;
	}

	template<class Allocator>
	void remove_values(Allocator&) {
		key_ends_here = false;
	}

	template<class Allocator>
	void copy_values_from(const node_type& other, Allocator&) {
		key_ends_here = other.key_ends_here;
		value_count = other.value_count;
	}

	friend bool operator < (const node_type& a, const node_type& b) {
		return a.key < b.key;
	}
	friend bool operator > (const node_type& a, const node_type& b) {
		return a.key > b.key;
	}
	friend bool operator == (const node_type& a, const node_type& b) {
		return a.key == b.key;
	}
};

} /* detail */
} /* tries */
} /* boost */

#endif
