target_sources(AnemoneRuntime
    PRIVATE
        "Aligned.hxx"
        "Bounds.hxx"
        "Easing.cxx"
        "Easing.hxx"
        "Fixed.hxx"
        "Functions.hxx"
        "Matrix3x2.hxx"
        "Matrix3x2.inl"
        "Numbers.hxx"
        "Packed.hxx"
        "Point.hxx"
        "Point.inl"
        "Random.cxx"
        "Random.hxx"
        "Rect.hxx"
        "Rect.inl"
        "Size.hxx"
        "Size.inl"
        "Thickness.hxx"
        "Thickness.inl"
        "Transform2.hxx"
        "Transform2.inl"
        "Types.hxx"
)

add_subdirectory("Detail")
