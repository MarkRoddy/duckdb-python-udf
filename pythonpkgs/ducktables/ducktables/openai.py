

import os, sys
import openai

openai.organization = os.getenv("OPENAI_ORG_ID")
openai.api_key = os.getenv("OPENAI_API_KEY")

from ducktables import ducktable

@ducktable(index = int, message = str, message_role = str, finish_reason = str)
def prompt(input_phrase, num_responses = 5):
    """
    SQL Usage:
    SELECT * FROM pytable('ducktables.openai:prompt', 'Write a limerick poem about how much you love SQL', 2);
    """
    completion = openai.ChatCompletion.create(
        model="gpt-3.5-turbo",
        # Complains when getting int like things so we explicitly cast
        n = int(num_responses),
        messages=[
            {"role": "user", "content": input_phrase}
            ]
        )
    for choice in completion.choices:
        yield(
            choice['index'],
            choice['message']['content'],
            choice['message']['role'],
            choice['finish_reason'],
            )

def main(args):
    for msg in prompt("Hello there!"):
        print(msg)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
