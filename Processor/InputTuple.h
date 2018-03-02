// (C) 2018 University of Bristol. See License.txt

/*
 * InputTuple.h
 *
 */

#ifndef PROCESSOR_INPUTTUPLE_H_
#define PROCESSOR_INPUTTUPLE_H_


template <class T>
struct InputTuple
{
  Share<T> share;
  T value;

  static int size()
    { return Share<T>::size() + T::size(); }

  static string type_string()
    { return T::type_string(); }

  void assign(const char* buffer)
    {
      share.assign(buffer);
      value.assign(buffer + Share<T>::size());
    }
};


template <class T>
struct RefInputTuple
{
  Share<T>& share;
  T& value;
  RefInputTuple(Share<T>& share, T& value) : share(share), value(value) {}
  void operator=(InputTuple<T>& other) { share = other.share; value = other.value; }
};


#endif /* PROCESSOR_INPUTTUPLE_H_ */
