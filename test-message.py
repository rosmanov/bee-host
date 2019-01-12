#!/usr/bin/env python3

import json
import struct
import sys

def main():
    text = """
aaaaQuoting @pavel.naumov's comment in HCSC-3424:
> I'm sure that we mustn't ignore the error how it is implemented in the PR but find the root of the bug.

So simply ignoring the fields with empty/missing `uuid` properties isn't enough.

I've used the following script to detect the assessment form fields without `uuid` properties:
```
foreach (\AssessmentForm::model()->findAll() as $form) {
  $layoutData = \AssessmentDecorator::parseLayoutData($form->data);
  foreach ($layoutData as $id => $item) {
    if (!isset($item->options->uuid)) {
      $errors++;
      fprintf(STDERR, "Form #%s Field #{$id} UUID: ( NONE ) item: %s\n", $form->id, $id, var_export($item, true));
    }
  }
}
echo "Errors: $errors\n";
```

It turned out, there was only one such field on HCSC UAT2:
```
Form #219 Field #0 UUID: ( NONE ) item: 0
```
The field (`item`) appears as a scalar `0` value! Now I wonder if we need to find out how it was possible to save such a field into the database, or just ignore it in the Assessment answer source.

sssss

a
    """
    editor = ""
    args = [ '-c',  ':set ft=markdown', '-c', ':set tw=12345']

    response = json.dumps({"text": text, "editor": editor, "args": args})
    sys.stdout.buffer.write(struct.pack('I', len(response)))
    # Write message itself
    sys.stdout.write(response)
    sys.stdout.flush()

    sys.exit(0)


if __name__ == '__main__':
    main()
