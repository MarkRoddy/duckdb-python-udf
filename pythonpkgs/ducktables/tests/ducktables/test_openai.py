

from ducktables import openai
from unittest import TestCase


class TestOpenAI(TestCase):

    def test_prompt_column_types(self):
        expected = (int, str, str, str)
        actual = openai.prompt.column_types()
        self.assertEqual(expected, actual)

    def test_prompt_column_names(self):
        expected = ('index', 'message', 'message_role', 'finish_reason')
        actual = openai.prompt.column_names()
        self.assertEqual(expected, actual)
        
