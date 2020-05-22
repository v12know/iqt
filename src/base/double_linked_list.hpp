#pragma once

#include <new>
#include <type_traits>

#include "recycling_pool.hpp"

namespace util {

    template<class T, bool IsPointer>
    struct list_node_impl {
        typedef T pointer_t;
    };

    template<class T>
    struct list_node_impl<T, false> {
        typedef T *pointer_t;
    };

    template<class T>
    struct list_node : list_node_impl<T, std::is_pointer<T>::value> {

        typedef list_node_impl<T, std::is_pointer<T>::value> base_t;
        typedef typename base_t::pointer_t pointer_t;
        typedef list_node<T> type;
        typedef type *list_node_ptr_t;

        list_node() = default;

        explicit list_node(pointer_t __data) : data_(__data), prev_(nullptr), next_(nullptr) {}

        ~list_node() = default;

        pointer_t data_;
        list_node_ptr_t prev_;
        list_node_ptr_t next_;
    };

    template<typename T>
    struct double_linked_list {

        typedef typename list_node<T>::type list_node_t;
        typedef typename list_node_t::pointer_t pointer_t;
        typedef typename list_node_t::list_node_ptr_t list_node_ptr_t;

        double_linked_list() : length_(0) {

            head_ = pool_.allocate(nullptr);
            tail_ = pool_.allocate(nullptr);

            head_->prev_ = tail_;
            head_->next_ = tail_;
            tail_->next_ = head_;
            tail_->prev_ = head_;
        }

        ~double_linked_list() = default;

        list_node_ptr_t head() {
            return head_;
        }

        list_node_ptr_t tail() {
            return tail_;
        }

        list_node_ptr_t front() const {
            return empty() ? nullptr : head_->next_;
        }

        list_node_ptr_t back() {
            return empty() ? nullptr : tail_->prev_;
        }

        list_node_ptr_t push_back(pointer_t __node_data) {

            list_node_ptr_t _node = pool_.allocate(__node_data);
            if (_node == nullptr) return _node;

            _node->data_ = __node_data;
            _node->next_ = tail_;
            list_node_ptr_t _back_node = back();

            if (nullptr == _back_node) {
                _node->prev_ = head_;
                head_->next_ = _node;
            } else {
                _node->prev_ = _back_node;
                _back_node->next_ = _node;
            }
            tail_->prev_ = _node;
            ++length_;

            return _node;
        }

        list_node_ptr_t push_front(pointer_t __node_data) {

            list_node_ptr_t _node = pool_.allocate(__node_data);
            if (_node == nullptr) throw std::bad_alloc();

            _node->data_ = __node_data;
            _node->prev_ = head_;
            list_node_ptr_t _front_node = front();
            if (nullptr == _front_node) {
                _node->next_ = tail_;
                tail_->prev_ = _node;
            } else {
                _node->next_ = _front_node;
                _front_node->prev_ = _node;
            }
            head_->next_ = _node;
            ++length_;
            return _node;
        }

        list_node_ptr_t front_insert(list_node_ptr_t __mark_node, pointer_t __node_data) {

            list_node_ptr_t _node;
            if (empty()) {
                _node = push_front(__node_data);
                return _node;
            }

            _node = pool_.allocate(__node_data);
            if (_node == nullptr) throw std::bad_alloc();
            _node->data_ = __node_data;

            list_node_ptr_t prev = __mark_node->prev_;

            prev->next_ = _node;
            _node->prev_ = prev;
            __mark_node->prev_ = _node;
            _node->next_ = __mark_node;

            ++length_;

            return _node;
        }

        list_node_ptr_t back_insert(list_node_ptr_t __mark_node, pointer_t __node_data) {

            list_node_ptr_t _node;
            if (empty()) {
                _node = push_back(__node_data);
                return _node;
            }

            _node = pool_.allocate(__node_data);
            if (_node == nullptr) throw std::bad_alloc();

            _node->data_ = __node_data;

            list_node_ptr_t next = __mark_node->next_;
            next->prev_ = _node;
            _node->next_ = next;
            __mark_node->next_ = _node;
            _node->prev_ = __mark_node;
            ++length_;
            return _node;
        }

        list_node_ptr_t pop_front() {

            if (empty()) return nullptr;

            list_node_ptr_t _front_node = front();
            _front_node->next_->prev_ = head_;
            head_->next_ = _front_node->next_;
            pool_.deallocate(_front_node);
            --length_;

            return _front_node;
        }

        list_node_ptr_t pop_back() {

            if (empty()) return nullptr;

            list_node_ptr_t _back_node = back();
            _back_node->prev_->next_ = tail_;
            tail_->prev_ = _back_node->prev_;
            pool_.deallocate(_back_node);
            --length_;

            return _back_node;
        }

        void erase(list_node_ptr_t __node) {
            list_node_ptr_t _prev = __node->prev_;
            list_node_ptr_t _next = __node->next_;
            _prev->next_ = _next;
            _next->prev_ = _prev;
            pool_.deallocate(__node);
            --length_;
        }

        bool empty() const { return length_ == 0; }

        std::size_t size() const { return length_; }

    private:
        list_node_ptr_t head_;
        list_node_ptr_t tail_;
        std::size_t length_;
        recycling_pool<list_node_t, 512> pool_;
    };

}