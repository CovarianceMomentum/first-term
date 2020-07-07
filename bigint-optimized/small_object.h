//
// Created by covariance on 07.07.2020.
//

#ifndef BIGINT_SMALL_OBJECT_H
#define BIGINT_SMALL_OBJECT_H
#pragma once

#include <algorithm>
#include <vector>
#include <cstdint>
#include "cow_wrapper.h"

class small_object {
// region fields
  static const size_t MAX_SIZE = 2;
  bool small;
  size_t small_size;
  union {
    uint32_t small_val[MAX_SIZE]{};
    cow_wrapper* data;
  };
// endregion

  void uniquify() {
    if (!data->unique()) {
      data = data->extract_unique();
    }
  }

  void desmall() {
    if (small) {
      uint32_t tmp[MAX_SIZE]{};
      std::copy_n(small_val, small_size, tmp);
      data = new cow_wrapper(0, 0);
      for (size_t i = 0; i != small_size; ++i) { data->push_back(tmp[i]); }
      small = false;
    }
  }

public:
  // region (cons/des)tructors
  small_object(size_t size, uint32_t val)
    : small(size <= MAX_SIZE)
    , small_size(size) {
    if (small) {
      for (size_t i = 0; i != small_size; ++i) { small_val[i] = val; }
      desmall();
    } else {
      data = new cow_wrapper(std::vector<uint32_t>(size, val));
    }
  }

  small_object(const small_object& that)
    : small(that.small)
    , small_size(that.small_size) {
    if (small) {
      std::copy_n(that.small_val, small_size, small_val);
      desmall();
    } else {
      that.data->add_ref();
      data = that.data;
    }
  }

  ~small_object() {
    if (!small) {
      if (data->unique()) {
        delete data;
        data = nullptr;
      } else {
        data->rem_ref();
      }
    }
  }
  // endregion

  // region accessors
  size_t size() const {
    return small ? small_size : data->size();
  }

  uint32_t operator[](size_t i) const {
    return small ? small_val[i] : (*data)[i];
  }

  uint32_t back() const {
    return small ? small_val[small_size - 1] : data->back();
  }
  // endregion

  // region changers
  small_object& operator=(const small_object& that) {
    if (this != &that) {
      if (this->small && that.small) {
        small_size = that.small_size;
        std::copy_n(that.small_val, small_size, small_val);
      }

      if (this->small && !that.small) {
        that.data->add_ref();
        data = that.data;
      }

      if (!this->small && that.small) {
        if (data->unique()) {
          delete data;
        } else {
          data->rem_ref();
        }

        small = that.small;
        small_size = that.small_size;
        std::copy_n(that.small_val, small_size, small_val);
      }

      if (!this->small && !that.small) {
        if (data->unique()) {
          delete data;
        } else {
          data->rem_ref();
        }
        that.data->add_ref();
        data = that.data;
      }
    }
    return *this;
  }

  void resize(size_t size, uint32_t val) {
    desmall();
    uniquify();
    data->resize(size, val);
  }

  uint32_t& operator[](size_t i) {
    desmall();
    uniquify();
    return (*data)[i];
  }

  void pop_back() {
    desmall();
    uniquify();
    data->pop_back();
  }

  void push_back(uint32_t val) {
    desmall();
    uniquify();
    data->push_back(val);
  }

  void reverse() {
    desmall();
    uniquify();
    data->reverse();
  }

  void insert(size_t len) {
    desmall();
    data->insert(len);
  }

  void erase(size_t len) {
    desmall();
    uniquify();
    data->erase(len);
  }
  // endregion
};

#endif //BIGINT_SMALL_OBJECT_H