
import inspect, sys
from typing import Any, Optional, Dict, Iterable

class DuckTableSchemaWrapper:

    def __init__(self, func, names = None, types = None):
        self.func = func
        self.types = types
        self.names = names

    def column_names(self, *args, **kwargs):
        if self.names:
            return self.names
        else:
            return self.names_from_types(*args, **kwargs)

    def names_from_types(self, *args, **kwargs):
        col_types = self.column_types(*args, **kwargs)
        if not col_types:
            return None
        return [f'column{i + 1}' for i in range(len(col_types))]

    def column_types(self, *args, **kwargs):
        if self.types:
            return self.types
        else:
            return self.types_from_signature()

    def types_from_signature(self):
        sig = inspect.signature(self.func)
        return_type = sig.return_annotation
        if sig.return_annotation == inspect.Signature.empty:
            return None

        # First level which represents the table
        if hasattr(return_type, '__origin__') and issubclass(return_type.__origin__, Iterable):
            row_type = return_type.__args__[0]

            # Second level which represents a row
            if hasattr(row_type, '__origin__') and issubclass(row_type.__origin__, Iterable):
                col_types = row_type.__args__  # Column types
                return list(col_types)
                # return {f'column{i + 1}': typ for i, typ in enumerate(col_types)}
        return None

    def __call__(self, *args, **kwargs):
        return self.func(*args, **kwargs)

def ducktable(*args, **kwargs):
    if (1 == len(args)) and (0 == len(kwargs)) and callable(args[0]):
        # No arguments, this is the decorator
        # We just return the decorated function
        return DuckTableSchemaWrapper(args[0])
    else:
        # User has supplied a set of arguments to the decorator when they
        # decoratored their function. This means they specified columns or
        # columns and types.
        if kwargs:
            names = tuple(kwargs.keys())
            types = tuple(kwargs.values())
        else:
            names = args
            types = None
        def decorator(func):
            return DuckTableSchemaWrapper(func, names, types)
        return decorator

