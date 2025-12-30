use std::{
    marker::PhantomData,
    ops::{Index, IndexMut},
};

use rand::distr::{Distribution, StandardUniform};

pub trait TypedIndex {
    fn typed_index(self) -> usize;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct TypedArray<T, const N: usize, I>
where
    I: TypedIndex,
{
    inner: [T; N],
    _phantom: PhantomData<I>,
}

impl<T, const N: usize, I> TypedArray<T, N, I>
where
    I: TypedIndex,
{
    pub const fn new(inner: [T; N]) -> Self {
        Self {
            inner,
            _phantom: PhantomData,
        }
    }
    pub fn into_inner(self) -> [T; N] {
        self.inner
    }

    pub const fn const_as_ref(&self) -> &[T; N] {
        &self.inner
    }
    pub const fn const_as_mut(&mut self) -> &mut [T; N] {
        &mut self.inner
    }
}

impl<IT, const N: usize, I> Distribution<TypedArray<IT, N, I>> for StandardUniform
where
    I: TypedIndex,
    StandardUniform: Distribution<[IT; N]>,
{
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> TypedArray<IT, N, I> {
        TypedArray::<IT, N, I>::new(rng.random())
    }
}

impl<T, const N: usize, I> Default for TypedArray<T, N, I>
where
    I: TypedIndex,
    T: Default,
{
    fn default() -> Self {
        Self {
            inner: core::array::from_fn(|_| Default::default()),
            _phantom: Default::default(),
        }
    }
}

impl<T, const N: usize, I> From<TypedArray<T, N, I>> for [T; N]
where
    I: TypedIndex,
    T: Default,
    [T; N]: Default,
{
    fn from(value: TypedArray<T, N, I>) -> Self {
        value.into_inner()
    }
}

impl<T, const N: usize, I> From<[T; N]> for TypedArray<T, N, I>
where
    I: TypedIndex,
{
    fn from(value: [T; N]) -> Self {
        Self::new(value)
    }
}

impl<T, const N: usize, I> AsRef<[T; N]> for TypedArray<T, N, I>
where
    I: TypedIndex,
{
    fn as_ref(&self) -> &[T; N] {
        &self.inner
    }
}
impl<T, const N: usize, I> AsMut<[T; N]> for TypedArray<T, N, I>
where
    I: TypedIndex,
{
    fn as_mut(&mut self) -> &mut [T; N] {
        &mut self.inner
    }
}

impl<T, const N: usize, I> Index<I> for TypedArray<T, N, I>
where
    I: TypedIndex,
{
    type Output = T;

    fn index(&self, index: I) -> &Self::Output {
        &self.inner[index.typed_index()]
    }
}

impl<T, const N: usize, I> IndexMut<I> for TypedArray<T, N, I>
where
    I: TypedIndex,
{
    fn index_mut(&mut self, index: I) -> &mut Self::Output {
        &mut self.inner[index.typed_index()]
    }
}
