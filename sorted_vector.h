/*
 * Шаблонный класс cim::sorted_vector
 *
 * - класс-обёртка для std::vector, имеющая методы для содержания вектора в
 * отсортированном по возрастанию виде, и реализующая бинарный поиск элементов
 * равных ключу, а также ближайших больших или меньших ключу элементов.
 *
 * Тип элемента, кроме удовлетворения требованиям, выдвигаемым std::vector,
 * должен быть сравниваемым, т.е. иметь реализованные операторы равенства
 * (==), больше (>) и меньше (<).
 *
 * Экземпляр sorted_vector может пребывать в двух состояниях: отсортированном
 * и (потенциально) испорченном (corrupted). По умолчанию, если не активна
 * соответствующая директива CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED,
 * любой неконстантный метод, предоставляющий доступ к элементам экземпляра,
 * переводит экземпляр в испорченное состояние.
 *
 * Для корректности работы методы остлеживают состояние экземпляра.
 * Неконстантные методы вызывают метод восстановления упорядоченности (repair),
 * если экземпляр находится в испорченном состоянии на момент начала работы
 * неконстантного метода, и такой вызов не приостановлен для экземпляра
 * (suspend_autorepair) или не отключён для всех шаблонов закомментированием
 * директивы CIM_SORTED_VECTOR_AUTOREPAIR.
 *
 * В случае утраты упорядоченности из-за одного элемента, метод repair
 * возвращает упорядоченность сдвигом части вектора и перестановкой
 * изменённого элемента в требуемое порядком место. Если упорядоченность
 * (потенциально) нарушена из-за большего числа элементов, запускается
 * std::sort. Для большего быстродействия рекомендуется приостанавливать
 * автоматическое восстановление упорядоченности методом suspend_autorepair
 * перед массированными добавлением или изменением элементов, после чего
 * возобновлять автоматическую сортировку вызовом метода resume_autorepair.
 *
 * Добавление элементов осуществляется методами push или replace. Метод replace
 * заменяет добавляемым первый найденный (он может быть не первым по счёту)
 * элемент, равный добавляемому. Если заменять нечего, то элемент просто
 * добавляется.
 *
 * Доступ к элементам осуществляется через оператор[], методы at, data и
 * violate. К первым и последним элементам - через front и back. Может быть
 * осуществлён доступ через итераторы.
 *
 * Методы поиска find_... осуществляют поиск в диапазоне [start_pos, end_pos]
 * (включая(!) end_pos), возвращая номер позиции (не итератор). Поскольку
 * методы find... константные, то перед их вызовом следует удостовериться в
 * том, что экземпляр находится в отсортированном состоянии, или вызвать
 * repair, иначе будет запущен медленный линейный поиск.
 *
 * При поиске в качестве аргумента передаётся константная ссылка
 * на экземпляр объекта, что может быть не удобным / медленным в случае,
 * если предварительно нужно вызвать конструктор объекта для поиска.
 * В этом случае вместо sorted_vector можно использовать шаблонный
 * класс sorted_vector_with_key, являющимся потомком класса sorted_vector.
 * При создании sorted_vector_with_key необходимо указать тип ключа
 * (поля, по которому происходит сравнение при сортировке) и указать
 * его имя в хранимом классе через макрос CIM_KEYNAME.
 *
 * Итераторы класса реализованы как связка указателя на экземпляр
 * sorted_vector и номер позиции в нём. Для них реализован только префиксный
 * инкремент.
 *
 */

#ifndef CIM_SORTED_VECTOR_H
#define CIM_SORTED_VECTOR_H

#define CIM_SORTED_VECTOR_AUTOREPAIR
  //В неконстантных методах запускается
  //процедура исправления (если изменён
  //только один элемент) / сортировки
  //при подозрительной порче

//#define CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  //Все методы будут работать с предположением
  //о том, что вектор всегда остортирован и он не
  //портится вызовом неконстантных методов или
  //операторов. Может быть актуально для работы
  //с классами, имеющими неизменяемый ключ, по
  //которому производится сортировка.

//#define CIM_SORTED_VECTOR_USE_MEMMOVE
  //Вместо перемещения с использованием
  //конструктора / оператора присваивания
  //будет произведено побитовое
  //перемещение. Иногда немного быстрее
  //обычного, но применимо только для
  //типов, допускающих такое копирование.

#include <vector>
#include <algorithm>
#include <iterator>

#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE
# include <cstring>
#endif

namespace cim{

template <class T>
  class sorted_vector_iterator;

template <class T>
  class sorted_vector_const_iterator;

template <class T>
  class sorted_vector
{
  public:
    sorted_vector();
    sorted_vector(const sorted_vector <T> &sv);
    sorted_vector(sorted_vector <T> &&sv);
    sorted_vector(std::initializer_list <T> ilist);
    sorted_vector(const std::vector <T> &v);
    sorted_vector(std::vector <T> &&v);

    virtual ~sorted_vector(){};

    sorted_vector <T> &operator=(const sorted_vector <T> &sv);
    sorted_vector <T> &operator=(sorted_vector <T> &&sv);

    sorted_vector <T> operator+(const sorted_vector <T> &sv) const;
    sorted_vector <T> operator+(const std::vector <T> &v) const;
    sorted_vector <T> operator+(const T &t) const;

    void operator+=(const sorted_vector <T> &sv);
    void operator+=(const std::vector <T> &v);
    void operator+=(const T &t);

  typedef sorted_vector_iterator <T>        iterator;
  typedef sorted_vector_const_iterator <T>  const_iterator;

    void assign(iterator first, iterator last);
    void assign(typename std::vector<T>::iterator first, typename std::vector<T>::iterator last);
    void assign(std::initializer_list <T> ilist);

    T       &at(size_t pos);
    const T &at(size_t pos) const;

    T       &operator[](size_t pos);
    const T &operator[](size_t pos) const;

    T       &front();
    const T &front() const;

    T       &back();
    const T &back() const;

    iterator        begin();
    const_iterator  begin()   const;
    const_iterator  cbegin()  const;

    iterator        end();
    const_iterator  end()     const;
    const_iterator  cend()    const;

    bool empty()      const;
    size_t size()     const;
    size_t max_size() const;

    void reserve(size_t n);

    size_t capacity() const;

    void shrink_to_fit();

    void clear();

    void erase(size_t pos);
    void erase(iterator first, iterator last);
    void erase(size_t pos_start, size_t pos_end);

    void push(const T &t);
    void push(T &&t);

    void replace(const T &t);
    void replace(T &&t);

    size_t find(const T &t, size_t start_pos = 0, size_t end_pos = -1)              const;

    size_t find_first(const T &t, size_t start_pos = 0, size_t end_pos = -1)        const;
    size_t find_last(const T &t, size_t start_pos = 0, size_t end_pos = -1)         const;
    size_t find_next(size_t current_pos, size_t end_pos = -1)                       const;
    size_t find_prev(size_t current_pos, size_t start_pos = 0)                      const;

    size_t find_linear(const T &t, size_t start_pos = 0, size_t end_pos = -1)       const;
    size_t find_linear_first(const T &t, size_t start_pos = 0, size_t end_pos = -1) const;
    size_t find_linear_last(const T &t, size_t start_pos = 0, size_t end_pos = -1)  const;

    size_t find_floor(const T &t, size_t start_pos = 0, size_t end_pos = -1)        const;
    size_t find_ceil(const T &t, size_t start_pos = 0, size_t end_pos = -1)         const;

    void sort();
    void repair();

    void merge(const sorted_vector <T> &sv);
    void merge(sorted_vector <T> &&sv);
    void merge(const std::vector <T> &v);
    void merge(std::vector <T> &&v);

    void merge_replace(const sorted_vector <T> &sv);
    void merge_replace(sorted_vector <T> &&sv);
    void merge_replace(const std::vector <T> &sv);
    void merge_replace(std::vector <T> &&sv);

    bool corrupted() const;

    std::vector <T> &storage();
    const std::vector <T> &cstorage();

    T *data();
    const T *data() const;

    void suspend_autorepair();
    void resume_autorepair();

  protected:
    void single_shift_left(size_t start_pos = 0, size_t end_pos = -1);
    void single_shift_right(size_t start_pos = 0, size_t end_pos = -1);

  private:
    std::vector <T> _storage;
    size_t          _last_modified            = (size_t)-1;
    bool            _is_corrupted             = false;
    bool            _flag_suspend_autorepair  = false;
};

template <class T>
  sorted_vector <T>:: sorted_vector() {};

template <class T>
  sorted_vector <T>:: sorted_vector(
    const sorted_vector <T> &sv)
{
  _storage = sv._storage;
  _last_modified = sv._last_modified;
  _is_corrupted = sv._is_corrupted;
  _flag_suspend_autorepair = sv._flag_suspend_autorepair;
}

template <class T>
  sorted_vector <T>:: sorted_vector(
    sorted_vector <T> &&sv)
{
  _storage = static_cast <std::vector <T> &&> (sv._storage);
  _last_modified = sv._last_modified;
  _is_corrupted = sv._is_corrupted;
  _flag_suspend_autorepair = sv._flag_suspend_autorepair;

  sv._last_modified = (size_t)-1;
  sv._is_corrupted = false;
  sv._flag_suspend_autorepair = false;
}

template <class T>
  sorted_vector <T>:: sorted_vector(
    std::initializer_list <T> ilist)
{
  assign(ilist);
}

template <class T>
  sorted_vector <T>:: sorted_vector(
    const std::vector <T> &v)
{
  _storage = v;
  sort();
}

template <class T>
  sorted_vector <T>:: sorted_vector(
    std::vector <T> &&v)
{
  _storage = static_cast <std::vector <T> &&>(v);
  sort();
}

template <class T>
  sorted_vector <T> &sorted_vector <T>::  operator=(
    const sorted_vector <T> &sv)
{
  if (this == &sv)
    return *this;

  _storage = sv._storage;
  _last_modified = sv._last_modified;
  _is_corrupted = sv._is_corrupted;
  _flag_suspend_autorepair = sv._flag_suspend_autorepair;

  return *this;
}

template <class T>
  sorted_vector <T> &sorted_vector <T>::  operator=(
    sorted_vector <T> &&sv)
{
  if (this == &sv)
    return *this;

  _storage = static_cast <std::vector <T> &&> (sv._storage);
  _last_modified = sv._last_modified;
  _is_corrupted = sv._is_corrupted;
  _flag_suspend_autorepair = sv._flag_suspend_autorepair;

  sv._last_modified = (size_t)-1;
  sv._is_corrupted = false;
  sv._flag_suspend_autorepair = false;

  return *this;
}

template <class T>
  sorted_vector <T> sorted_vector <T>:: operator+(
    const sorted_vector <T> &sv)
    const
{
  sorted_vector <T> rsv(*this);
  rsv.merge(sv);
  return rsv;
}

template <class T>
  sorted_vector <T> sorted_vector <T>:: operator+(
    const std::vector <T> &v)
    const
{
  sorted_vector <T> rsv(*this);
  rsv.merge(v);
  return rsv;
}

template <class T>
  sorted_vector <T> sorted_vector <T>:: operator+(
    const T &t)
    const
{
  sorted_vector <T> rsv(*this);
  rsv.push(t);
  return rsv;
}

template <class T>
  void sorted_vector <T>:: operator+=(
    const sorted_vector <T> &sv)
{
  merge(sv);
}

template <class T>
  void sorted_vector <T>:: operator+=(
    const std::vector <T> &v)
{
  merge(v);
}

template <class T>
  void sorted_vector <T>:: operator+=(
    const T &t)
{
  push(t);
}

template <class T>
  void sorted_vector <T>::  assign(
    iterator first,
    iterator last)
{
  if (  (first._owner == last._owner)
      &&(first._owner != nullptr))
  {

    _storage.assign(
      first._owner->_storage.begin() + first._pos,
      first._owner->_storage.begin() + last._pos);

    sort();
  }

}

template <class T>
  void sorted_vector <T>::  assign(
    typename std::vector<T>::iterator first,
    typename std::vector<T>::iterator last)
{
  _storage.assign(first, last);
  sort();
}

template <class T>
  void sorted_vector <T>::  assign(
    std::initializer_list <T> ilist)
{
  _storage.assign(ilist);
  sort();
}

template <class T>
  T &sorted_vector <T>::  at(
    size_t pos)
{
  return _storage.at(pos);
}

template <class T>
  const T &sorted_vector <T>::  at(
    size_t pos)
    const
{
  return _storage.at(pos);
}

template <class T>
  T &sorted_vector <T>:: operator[](
    size_t pos)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
  if (!_flag_suspend_autorepair)
    repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  if (_is_corrupted)
    _last_modified = -1;
  else
    _last_modified = pos;
  _is_corrupted = true;
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED

  return _storage[pos];
}

template <class T>
  const T &sorted_vector <T>:: operator[](
    size_t pos)
    const
{
  return static_cast <const T &> (_storage[pos]);
}

template <class T>
  T &sorted_vector <T>:: front()
{
  return (*this)[0];
}

template <class T>
  const T &sorted_vector <T>:: front()
  const
{
  return (*this)[0];
}

template <class T>
  T &sorted_vector <T>:: back()
{
  return (*this)[_storage.size() - 1];
}

template <class T>
  const T &sorted_vector <T>:: back()
  const
{
  return (*this)[_storage.size() - 1];
}

template <class T>
  typename sorted_vector <T>::iterator sorted_vector <T>:: begin()
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
  if (!_flag_suspend_autorepair)
    repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  if (_is_corrupted)
    _last_modified = -1;
  else
    _last_modified = 0;
  _is_corrupted = true;
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  return iterator(0, this);
}

template <class T>
  typename sorted_vector <T>::const_iterator sorted_vector <T>::  begin()
  const
{
  return cbegin();
}

template <class T>
  typename sorted_vector <T>::const_iterator sorted_vector <T>::  cbegin()
  const
{
  return const_iterator(0, this);
}

template <class T>
  typename sorted_vector <T>::iterator sorted_vector <T>:: end()
{
  return iterator(size(), this);
}

template <class T>
  typename sorted_vector <T>::const_iterator sorted_vector <T>::  end()
  const
{
  return cend();
}

template <class T>
  typename sorted_vector <T>::const_iterator sorted_vector <T>::  cend()
  const
{
  return const_iterator(size(), this);
}

template <class T>
  bool sorted_vector <T>::  empty()
    const
{
  return _storage.empty();
}

template <class T>
  size_t sorted_vector <T>:: size()
    const
{
  return _storage.size();
}

template <class T>
  size_t sorted_vector <T>::  max_size()
  const
{
  return _storage.max_size();
}

template <class T>
  void sorted_vector <T>::  reserve(
    size_t n)
{
  _storage.reserve(n);
}

template <class T>
  size_t sorted_vector <T>::  capacity()
    const
{
  return _storage.capacity();
}

template <class T>
  void sorted_vector <T>::  shrink_to_fit()
{
  _storage.shrink_to_fit();
}

template <class T>
  void sorted_vector <T>:: clear()
{
  _storage.clear();
  _last_modified = (size_t)-1;
  _is_corrupted = false;
}

template <class T>
  void sorted_vector <T>:: erase(
    size_t pos)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED

#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
  _storage.erase(_storage.begin() + pos);
#else
  single_shift_left(pos);
  _storage.pop_back();
#endif
}

template <class T>
  void sorted_vector <T>::  erase(
    iterator first,
    iterator last)
{
  if (  (first._owner == this)
      &&(last._owner == this))
    erase(first.pos(), last.pos() - 1);
}

template <class T>
  void sorted_vector <T>::  erase(
    size_t pos_start,
    size_t pos_end)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
#ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
#endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED

  _storage.erase(
    _storage.begin() + pos_start,
    _storage.begin() + pos_end + 1);
}

template <class T>
  void sorted_vector <T>:: push(
    const T &t)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
#ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
#endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (!_is_corrupted) {
    if (_storage.size() == 0) {
      _storage.push_back(t);
      return;
    }
    size_t pos = find_ceil(t);
    if (pos == (size_t)-1)
      _storage.push_back(t);
    else {
#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
      if (_storage[pos] == t)

        _storage.insert(
          _storage.begin() + pos + 1,
          t);

      else

        _storage.insert(
          _storage.begin() + pos,
          t);

#else
      _storage.push_back(t);
      char buf[sizeof(T)];
      if (_storage[pos] == t) {

        memcpy(
          reinterpret_cast <void *> (buf),
          reinterpret_cast <void *> (&_storage.back()),
          sizeof(T));

        single_shift_right(pos + 1);

        memcpy(
          reinterpret_cast <void *> (&_storage[pos + 1]),
          reinterpret_cast <void *> (&buf),
          sizeof(T));

      } else {

        memcpy(
          reinterpret_cast <void *> (buf),
          reinterpret_cast <void *> (&_storage.back()),
          sizeof(T));

        single_shift_right(pos);

        memcpy(
          reinterpret_cast <void *> (&_storage[pos]),
          reinterpret_cast <void *> (&buf),
          sizeof(T));
      }
#endif
    }
  } else {
    _storage.push_back(t);
  }
}

template <class T>
  void sorted_vector <T>:: push(
    T &&t)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (!_is_corrupted) {
    if (_storage.size() == 0) {
      _storage.push_back(static_cast <T &&> (t));
      return;
    }
    size_t pos = find_ceil(t);
    if (pos == -1)
      _storage.push_back(static_cast <T &&> (t));
    else
    {
#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
      if (_storage[pos] == t)

        _storage.insert(
          _storage.begin() + pos + 1,
          static_cast <T &&> (t));

      else

        _storage.insert(
          _storage.begin() + pos,
          static_cast <T &&> (t));

#else
      _storage.push_back(static_cast <T &&> (t));

      char buf[sizeof(T)];

      if (_storage[pos] == t)
      {
        memcpy(
          reinterpret_cast <void *> (buf),
          reinterpret_cast <void *> (&_storage.back()),
          sizeof(T));

        single_shift_right(pos + 1);

        memcpy(
          reinterpret_cast <void *> (&_storage[pos + 1]),
          reinterpret_cast <void *> (&buf),
          sizeof(T));

      } else {

        memcpy(
          reinterpret_cast <void *> (buf),
          reinterpret_cast <void *> (&_storage.back()),
          sizeof(T));

        single_shift_right(pos);

        memcpy(
          reinterpret_cast <void *> (&_storage[pos]),
          reinterpret_cast <void *> (&buf),
          sizeof(T));
      }
#endif
    }
  } else {  //corrupted
    _storage.push_back(static_cast <T &&> (t));
  }
}

template <class T>
  void sorted_vector <T>:: replace(
    const T &t)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  size_t pos = find(t);
  if (pos != (size_t)-1)
    _storage[pos] = t;
  else
    push(t);
}

template <class T>
  void sorted_vector <T>:: replace(
    T &&t)
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  size_t pos = find(t);
  if (pos != (size_t)-1)
    _storage[pos] = static_cast <T &&>(t);
  else
    push(static_cast <T &&>(t));
}

template <class T>
  size_t sorted_vector <T>:: find(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (_storage.empty())
    return -1;
  if (!_is_corrupted) {
    size_t f = start_pos;
    size_t l = (end_pos == (size_t)-1) ? _storage.size() - 1 : end_pos;
    size_t m = (f + l) / 2;
    while (f <= l) {
      if (_storage[m] == t)
        return m;
      else if (_storage[f] == t)
        return f;
      else if (_storage[l] == t)
        return l;
      else {
        if (_storage[m] < t) {
          f = m + 1;
        } else {
          {
            if (m == 0)
              return -1;
            l = m - 1;
          }
        }
        m = (l + f) / 2;
      }
    }
    return -1;
  } else {
    return find_linear(t, start_pos, end_pos);
  }
}

template <class T>
  size_t sorted_vector <T>:: find_first(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (_storage.empty())
    return -1;
  if (end_pos == -1)
    end_pos = _storage.size() - 1;
  if (!_is_corrupted) {
    size_t pos = find(t, start_pos, end_pos);
    if (pos == -1)
      return -1;
    size_t prev_pos;
    while (   (t == _storage[pos])
           && (pos >= start_pos))
      prev_pos = pos--;
    return prev_pos;
  } else {
    return find_linear_first(t, start_pos, end_pos);
  }
}

template <class T>
  size_t sorted_vector <T>:: find_last(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (end_pos == -1)
    end_pos = _storage.size() - 1;
  if (!_is_corrupted) {
    size_t pos = find(t, start_pos, end_pos);
    if (pos == -1)
      return -1;
    size_t prev_pos;
    while (   (t == _storage[pos])
           && (pos <= end_pos))
      prev_pos = pos++;
    return prev_pos;
  } else {
    return find_linear_last(t);
  }
}

template <class T>
  size_t sorted_vector <T>:: find_next(
    size_t current_pos,
    size_t end_pos)
    const
{
  if (end_pos == -1)
    end_pos = _storage.size() - 1;
  if (current_pos == end_pos)
    return -1;
  if (!_is_corrupted) {
    if (  _storage[current_pos]
        ==_storage[current_pos + 1])
      return ++current_pos;
    else
      return -1;
  } else {
    return find_first(_storage[current_pos], current_pos + 1, end_pos);
  }
}

template <class T>
  size_t sorted_vector <T>:: find_prev(
    size_t current_pos,
    size_t start_pos)
    const
{
  if (current_pos <= start_pos)
    return -1;
  if (!_is_corrupted)
  {
    if (  _storage[current_pos]
        ==_storage[current_pos - 1])
      return --current_pos;
    else
      return -1;
  } else {

    return find_last(
      _storage[current_pos],
      start_pos,
      current_pos - 1);
  }
}

template <class T>
  size_t sorted_vector <T>:: find_linear(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (size() == 0)
    return (size_t)-1;

  if (end_pos == (size_t)-1)
    end_pos = size() - 1;

  if (start_pos > end_pos)
    return -1;

  if (end_pos == (size_t)-1)
    end_pos = _storage.size() - 1;

  while (start_pos != end_pos)
  {
    if (_storage[start_pos++] == t)
      return start_pos - 1;
  }
  return -1;
}

template <class T>
  size_t sorted_vector <T>::  find_linear_first(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (size() == 0)
    return -1;

  if (end_pos == (size_t)-1)
    end_pos = size() - 1;

  if (start_pos > end_pos)
    return -1;

  if (end_pos == (size_t)-1)
    end_pos = _storage.size() - 1;

  while (start_pos != end_pos)
  {
    if (_storage[start_pos++] == t)
      return start_pos - 1;
  }
  return -1;
}

template <class T>
  size_t sorted_vector <T>::  find_linear_last(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (size() == 0)
    return -1;

  if (end_pos == (size_t)-1)
    end_pos = size() - 1;

  if (start_pos > end_pos)
    return -1;

  if (end_pos == (size_t)-1)
    end_pos = _storage.size() - 1;

  while (start_pos != end_pos)
  {
    if (_storage[end_pos--] == t)
      return end_pos + 1;
  }
  return -1;
}

template <class T>
  size_t sorted_vector <T>:: find_floor(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (_storage.empty())
    return -1;
  end_pos = (end_pos == (size_t)-1 ? _storage.size() - 1 : end_pos);
  if (!_is_corrupted) {
    if (_storage[start_pos] > t)
      return -1;
    if (_storage[end_pos] < t)
      return end_pos;
    size_t f = start_pos;
    size_t l = end_pos;
    size_t m = (f + l) / 2;
    while (f < l) {
      if (_storage[m] == t) {
        while (m-- > f) {
          if (!(_storage[m] == t))
            return m + 1;
        }
        return f;
      }
      if (_storage[m] < t)
        f = m + 1;
      else
        l = m - 1;
      m = (l + f) / 2;
    }
    if (_storage[m] == t)
      return m;
    else
    if (_storage[m] < t)
      return m;
    else
      return m - 1;
  } else {  //is_corrupted
    return -1;
  }
}

template <class T>
  size_t sorted_vector <T>:: find_ceil(
    const T &t,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (_storage.empty())
    return -1;
  end_pos = (end_pos == (size_t)-1 ? _storage.size() - 1 : end_pos);
  if (!_is_corrupted)
  {
    if (_storage[0] > t)
      return 0;
    if (_storage[end_pos] < t)
      return -1;
    size_t f = start_pos;
    size_t l = end_pos;
    size_t m = (f + l) / 2;
    while (f < l) {
      if (_storage[m] == t) {
        while (m++ < l) {
          if (!(_storage[m] == t))
            return m - 1;
        }
        return l;
      }
      if (_storage[m] < t)
        f = m + 1;
      else
        l = m - 1;
      m = (f + l) / 2;
    }
    if (_storage[m] == t)
      return m;
    else if (_storage[m] > t)
      return m;
    else
      return m + 1;
  } else {
    return -1;
  }
}

template <class T>
  void sorted_vector <T>:: sort()
{
  std::sort(_storage.begin(), _storage.end());
  _is_corrupted = false;
}

template <class T>
  void sorted_vector <T>:: repair()
{
  if (_is_corrupted) {
    if (_last_modified == (size_t)-1)
      sort();
    else {
      _is_corrupted = false;

      if (  (_last_modified > 0)
          &&(   _storage[_last_modified]
              < _storage[_last_modified - 1])) {

        size_t pos_ceil = find_ceil(
          _storage[_last_modified],
          0,
          _last_modified-1);

#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
        T t(static_cast <T &&> (_storage[_last_modified]));
#endif
#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE
        char t[sizeof(T)];
        memcpy(t, &_storage[_last_modified], sizeof(T));
#endif
        single_shift_right(pos_ceil, _last_modified);
#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
        _storage[pos_ceil] = static_cast <T &&> (t);
#endif
#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE
        memcpy(&_storage[pos_ceil], t, sizeof(T));
#endif

      } else

      if (  (_last_modified < _storage.size() - 1)
          &&(   _storage[_last_modified]
              > _storage[_last_modified + 1])) {

        size_t pos_floor = find_floor(
          _storage[_last_modified],
          _last_modified + 1,
          _storage.size() - 1);

#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
        T t(static_cast <T &&> (_storage[_last_modified]));
#endif
#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE
        char t[sizeof(T)];
        memcpy(t, &_storage[_last_modified], sizeof(T));
#endif
        single_shift_left(_last_modified, pos_floor);
#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
        _storage[pos_floor] = static_cast <T &&> (t);
#endif
#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE
        memcpy(&_storage[pos_floor], t, sizeof(T));
#endif
      }
    }
    _last_modified = -1;
  }
}

template <class T>
  void sorted_vector <T>::  merge(
    const sorted_vector <T> &sv)
{
  _storage.reserve(sv.size() + size());
  _storage.insert(_storage.end(), sv._storage.begin(), sv._storage.end());
  _last_modified = (size_t)-1;
  _is_corrupted = true;
  repair();
}

template <class T>
  void sorted_vector <T>::  merge(
    sorted_vector <T> &&sv)
{
  merge(static_cast <std::vector<T> &&> (sv._storage));
}

template <class T>
  void sorted_vector <T>::  merge(
    const std::vector <T> &v)
{
  _storage.reserve(v.size() + size());
  _storage.insert(_storage.end(), v.begin(), v.end());
  _last_modified = (size_t)-1;
  _is_corrupted = true;
  repair();
}

template <class T>
  void sorted_vector <T>::  merge(
    std::vector <T> &&v)
{
  _storage.reserve(v.size() + size());

  _storage.insert(
    _storage.end(),
    std::make_move_iterator(v.begin()),
    std::make_move_iterator(v.end()));

  _last_modified = (size_t)-1;
  _is_corrupted = true;
  repair();
}

template <class T>
  void sorted_vector <T>::  merge_replace(
    const sorted_vector <T> &sv)
{
  _storage.reserve(sv.size() + size());
  for (size_t i = 0; i < sv.size(); i++)
    replace(sv[i]);
}

template <class T>
  void sorted_vector <T>::  merge_replace(
    sorted_vector <T> &&sv)
{
  _storage.reserve(sv.size() + size());
  for (size_t i = 0; i < sv.size(); i++)
    replace(static_cast <T &&> (sv[i]));
}

template <class T>
  void sorted_vector <T>::  merge_replace(
    const std::vector <T> &v)
{
  _storage.reserve(v.size() + size());
  for (size_t i = 0; i < v.size(); i++)
    replace(v[i]);
}

template <class T>
  void sorted_vector <T>::  merge_replace(
    std::vector <T> &&v)
{
  _storage.reserve(v.size() + size());
  for (size_t i = 0; i < v.size(); i++)
    replace(static_cast <T &&> (v[i]));
}

template <class T>
  bool sorted_vector <T>:: corrupted()
  const
{
  return _is_corrupted;
}

template <class T>
  std::vector <T> &sorted_vector <T>::  storage()
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
#ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
#endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
  _is_corrupted = true;
  _last_modified = -1;
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  return _storage;
}

template <class T>
  const std::vector <T> &sorted_vector <T>::  cstorage()
{
  return _storage;
}

template <class T>
  T *sorted_vector <T>::  data()
{
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  if (_is_corrupted) {
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
    if (!_flag_suspend_autorepair)
      repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
  }
  _is_corrupted = true;
  _last_modified = -1;
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  return _storage.data();
}

template <class T>
  const T *sorted_vector <T>::  data()
  const
{
  return _storage.data();
}

template <class T>
  void sorted_vector <T>::  suspend_autorepair()
{
  _flag_suspend_autorepair = true;
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
  _last_modified = -1;
  _is_corrupted = true;
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
}

template <class T>
  void sorted_vector <T>::  resume_autorepair()
{
  _flag_suspend_autorepair = false;
#ifndef CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
# ifdef CIM_SORTED_VECTOR_AUTOREPAIR
  repair();
# endif // CIM_SORTED_VECTOR_AUTOREPAIR
#endif // CIM_SORTED_VECTOR_IGNORE_POSSIBLY_CORRUPTED
}

//***protected methods***

template <class T>
  void sorted_vector <T>::  single_shift_left(
    size_t start_pos,
    size_t end_pos)
{
  if (start_pos == end_pos)
    return;

  if (end_pos == (size_t)-1)
    end_pos = size() - 1;

#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
  T t = static_cast <T &&> (_storage[start_pos]);
  _storage.erase(_storage.begin() + start_pos);

  _storage.insert(
    _storage.begin() + end_pos,
    static_cast <T &&> (t));

#endif // !CIM_SORTED_VECTOR_USE_MEMMOVE

#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE

  memmove(
    reinterpret_cast <void *> (&_storage[start_pos]),
    reinterpret_cast <void *> (&_storage[start_pos + 1]),
    sizeof(T) * (end_pos - start_pos));

#endif // CIM_SORTED_VECTOR_USE_MEMMOVE
}

template <class T>
  void sorted_vector <T>::  single_shift_right(
    size_t start_pos,
    size_t end_pos)
{
  if (start_pos == end_pos)
    return;

  if (end_pos == (size_t)-1)
    end_pos = size() - 1;
#ifndef CIM_SORTED_VECTOR_USE_MEMMOVE
  T t = static_cast <T &&> (_storage[end_pos]);
  _storage.erase(_storage.begin() + end_pos);

  _storage.insert(
    _storage.begin() + start_pos,
    static_cast <T &&> (t));
#endif // !CIM_SORTED_VECTOR_USE_MEMMOVE
#ifdef CIM_SORTED_VECTOR_USE_MEMMOVE
  memmove(
    reinterpret_cast <void *> (&_storage[start_pos + 1]),
    reinterpret_cast <void *> (&_storage[start_pos]),
    sizeof(T) * (end_pos - start_pos));
#endif // !CIM_SORTED_VECTOR_USE_MEMMOVE
}

//***end sorted_vector***

//***Iterators***

template <class T>
  class sorted_vector_iterator : public std::iterator<std::input_iterator_tag, T>
{
  friend sorted_vector <T>;

  typedef T & reference;

  sorted_vector_iterator(size_t pos, sorted_vector <T> *owner);

  public:
    sorted_vector_iterator(const sorted_vector_iterator &it);

    bool operator!=(const sorted_vector_iterator &it) const;
    bool operator==(const sorted_vector_iterator &it) const;

    typename sorted_vector_iterator::reference operator*() const;

    sorted_vector_iterator &operator++();

    sorted_vector_iterator operator+(size_t inc) const;
    sorted_vector_iterator operator-(size_t dec) const;

    size_t pos() const;

  private:
    size_t _pos = (size_t)-1;
    sorted_vector <T> *_owner = nullptr;
};

template <class T>
  sorted_vector_iterator <T>::  sorted_vector_iterator(
    size_t pos,
    sorted_vector <T> *owner)
    : _pos(pos), _owner(owner)
{}

template <class T>
  sorted_vector_iterator <T>::  sorted_vector_iterator(
    const sorted_vector_iterator &it)
    : _pos(it._pos), _owner(it._owner)
{}


template <class T>
  bool sorted_vector_iterator <T>:: operator!=(
    const sorted_vector_iterator &it)
    const
{
  return (  (_owner != it._owner)
          ||(_pos != it._pos));
}

template <class T>
  bool sorted_vector_iterator <T>:: operator==(
    const sorted_vector_iterator &it)
    const
{
  return (  (_owner == it._owner)
          &&(_pos == it._pos));
}

template <class T>
  typename sorted_vector_iterator <T>::reference
  sorted_vector_iterator <T>:: operator*()
  const
{
  return _owner->operator[](_pos);
}

template <class T>
  sorted_vector_iterator <T> &sorted_vector_iterator <T>::  operator++()
{
  _pos++;
  return *this;
}

template <class T>
  sorted_vector_iterator <T> sorted_vector_iterator <T>:: operator+(
  size_t inc)
  const
{
  return sorted_vector_iterator(_pos + inc, _owner);
}

template <class T>
  sorted_vector_iterator <T> sorted_vector_iterator <T>:: operator-(
  size_t dec)
  const
{
  return sorted_vector_iterator(_pos - dec, _owner);
}

template <class T>
  size_t sorted_vector_iterator <T>:: pos()
    const
{
  return _pos;
}

template <class T>
  class sorted_vector_const_iterator : public std::iterator<std::input_iterator_tag, T>
{
  friend sorted_vector <T>;

  typedef const T & const_reference;

  sorted_vector_const_iterator(size_t pos, const sorted_vector <T> *owner);

  public:
    sorted_vector_const_iterator(const sorted_vector_const_iterator &it);

    bool operator!=(const sorted_vector_const_iterator &it) const;
    bool operator==(const sorted_vector_const_iterator &it) const;

    typename sorted_vector_const_iterator::const_reference operator*() const;

    sorted_vector_const_iterator &operator++();

    sorted_vector_const_iterator operator+(size_t inc) const;
    sorted_vector_const_iterator operator-(size_t dec) const;

    size_t pos() const;

  private:
    size_t _pos = (size_t)-1;
    const sorted_vector <T> *_owner = nullptr;
};


template <class T>
  sorted_vector_const_iterator <T>::  sorted_vector_const_iterator(
    size_t pos,
    const sorted_vector <T> *owner)
    : _pos(pos), _owner(owner)
{}

template <class T>
  sorted_vector_const_iterator <T>::  sorted_vector_const_iterator(
    const sorted_vector_const_iterator &it)
    : _pos(it._pos), _owner(it._owner)
{}


template <class T>
  bool sorted_vector_const_iterator <T>:: operator!=(
    const sorted_vector_const_iterator &it)
    const
{
  return (  (_owner != it._owner)
          ||(_pos != it._pos));
}

template <class T>
  bool sorted_vector_const_iterator <T>:: operator==(
    const sorted_vector_const_iterator &it)
    const
{
  return (  (_owner == it._owner)
          &&(_pos == it._pos));
}

template <class T>
  typename sorted_vector_const_iterator <T>::const_reference
  sorted_vector_const_iterator <T>:: operator*()
  const
{
  return _owner->operator[](_pos);
}

template <class T>
  sorted_vector_const_iterator <T> &sorted_vector_const_iterator <T>::  operator++()
{
  _pos++;
  return *this;
}

template <class T>
  sorted_vector_const_iterator <T> sorted_vector_const_iterator <T>:: operator+(
    size_t inc)
    const
{
  return sorted_vector_const_iterator(_pos + inc, _owner);
}

template <class T>
  sorted_vector_const_iterator <T> sorted_vector_const_iterator <T>:: operator-(
    size_t dec)
    const
{
  return sorted_vector_const_iterator(_pos - dec, _owner);
}

template <class T>
  size_t sorted_vector_const_iterator <T>:: pos()
    const
{
  return _pos;
}

#define CIM_KEYNAME _key
  //Макрос определяет название поля в классе,
  //хранимом sorted_vector_with_key, по
  //которому происходит сравнение для поиска
  //без вызова конструктора хранимого класса.
  // !Должно совпадать с полем, по которому !
  // !происходит сортировка!                !

template <class T, class Key>
  class sorted_vector_with_key : public sorted_vector <T>
{
  public:
    size_t find(const Key &key, size_t start_pos = 0, size_t end_pos = -1)        const;
    size_t find_linear(const Key &key, size_t start_pos = 0, size_t end_pos = -1) const;

    using sorted_vector <T>::find;
    using sorted_vector <T>::find_linear;
};

template <class T, class Key>
  size_t sorted_vector_with_key <T, Key>:: find(
    const Key &key,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (this->size() == 0)
    return -1;
  if (!this->corrupted()) {
    size_t f = start_pos;
    size_t l = (end_pos == (size_t)-1) ? this->size() - 1 : end_pos;
    size_t m = (f + l) / 2;
    while (f <= l) {
      if (this->operator[](m).CIM_KEYNAME == key)
        return m;
      else if (this->operator[](f).CIM_KEYNAME == key)
        return f;
      else if (this->operator[](l).CIM_KEYNAME == key)
        return l;
      else {
        if (this->operator[](m).CIM_KEYNAME < key) {
          f = m + 1;
        } else {
          {
            if (m == 0)
              return -1;
            l = m - 1;
          }
        }
        m = (l + f) / 2;
      }
    }
    return -1;
  } else {
    return find_linear(key, start_pos, end_pos);
  }
}

template <class T, class Key>
  size_t sorted_vector_with_key <T, Key>:: find_linear(
    const Key &key,
    size_t start_pos,
    size_t end_pos)
    const
{
  if (this->size() == 0)
    return (size_t)-1;

  if (end_pos == (size_t)-1)
    end_pos = this->size() - 1;

  if (start_pos > end_pos)
    return (size_t)-1;

  if (end_pos == (size_t)-1)
    end_pos = this->size() - 1;

  while (start_pos != end_pos)
  {
    if (this->operator[](start_pos++).CIM_KEYNAME == key)
      return start_pos - 1;
  }
  return (size_t)-1;
}

}

#endif // CIM_SORTED_VECTOR_H
