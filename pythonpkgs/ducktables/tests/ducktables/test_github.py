
from unittest import TestCase
from ducktables import github


class TestGithub(TestCase):
    def test_schema(self):
        # @ducktable(name = str, description = str, language = str)
        actual_names = github.repos_for.column_names()
        expected_names = ('repo', 'description', 'language')
        self.assertEqual(expected_names, actual_names)

        actual_types = github.repos_for.column_types()
        expected_types = (str, str, str)
        self.assertEqual(expected_types, actual_types)

