#! /usr/bin/env python3

import re
import sys
import textwrap


class ParsingException(Exception):
    pass


def convert_variable(variable_name: str, default_value: str, comment: str, indentation: int = None):
    if indentation is None:
        indentation = 0

    variable_name_upper = variable_name.upper()
    env_var_name = ".Env.%s" % variable_name_upper

    lines = []
    lines.append("{{ if (contains .Env \"%s\") -}}" % variable_name_upper)
    #lines.append(comment)
    #lines.append("// default/example value: %s" % default_value)
    lines.append("%s \"{{ %s }}\"" % (variable_name, env_var_name))
    lines.append("{{ else -}}")
    lines.append("// %s %s  %s" % (variable_name, default_value, comment))
    lines.append("{{ end -}}")
    lines.append("")

    return textwrap.indent("\n".join(lines), " "*indentation)


def parse_and_convert_variable(pattern: str, line: str, indentation: int = None):
    match = re.match(pattern, line)

    if not match:
        return

    return convert_variable(*match.groups(), indentation)


def parse_regular_variable(line: str):
    return parse_and_convert_variable(r"^//\s?((?:sv_|admin|server)\w+)\s+(.+)\s+(\/\/.*)$", line)


def parse_rehashing_variable(line: str):
    return parse_and_convert_variable(r"^    //\s*((?!irc)\w+)\s+(.+)\s+(\/\/.*)$", line, 4)


def parse_add_variable(line: str):
    match = re.match(r"^//\s? (add\w+)\s+(.+)\s*(|\/\/.*)$", line)

    if not match:
        return

    variable_name, default_value, comment = match.groups()

    variable_name_upper = variable_name.upper()
    env_var_name = ".Env.%s" % variable_name_upper

    lines = []
    lines.append("{{ if (contains .Env \"%s\") -}}" % variable_name_upper)
    #lines.append(comment)
    #lines.append("// default/example value: %s" % default_value)
    lines.append("{{ range $e := ( split %s \";\" ) -}}" % env_var_name)
    lines.append("%s {{ $e }}" % variable_name)
    lines.append("{{ end -}}")
    lines.append("{{ else -}}")
    lines.append(line)
    lines.append("{{ end -}}")
    lines.append("")

    return "\n".join(lines)


def parse_and_convert_line(line: str) -> str:
    for f in [parse_regular_variable, parse_rehashing_variable, parse_add_variable]:
        parsed = f(line)

        if parsed is not None:
            return parsed

    else:
        return line


def make_irc_section():
    return textwrap.dedent(
    """
    {{ if (contains .Env "ENABLE_IRC") -}}
    // special single-server IRC configuration, suitable for our Docker deployment
    // setting ENABLE_IRC to some value will be sufficient in most cases
    if (= $rehashing 0) [
        ircfilter {{ default .Env.IRC_FILTER "1" }} // defines the way the colour-to-irc filter works; 0 = off, "1" = convert, 2 = strip

        ircaddrelay ircrelay {{ default .Env.IRC_RELAY_HOSTNAME "localhost" }} {{ default .Env.IRC_RELAY_PORT "6667" }} {{ default .Env.IRC_RELAY_NICK "re-server" }}

        {{ if (contains .Env "IRC_BIND_ADDRESS") -}}
        ircbind ircrelay {{ .Env.IRC_BIND_ADDRESS }} // use this only if you need to bind to a specific address, eg. multihomed machines
        {{ end -}}

        {{ if (contains .Env "IRC_SERVER_PASS" ) -}}
        ircpass ircrelay {{ .Env.IRC_SERVER_PASS }} // some networks can use the PASS field to identify to nickserv
        {{ end -}}

        {{ if (contains .Env "IRC_CHANNELS") -}}
        {{ range $e := ( split .Env.IRC_CHANNELS "," ) -}}
        ircaddchan ircrelay "{{ $e }}"
        ircrelaychan ircrelay "{{ $e }}" 3
        {{ end -}}
        {{ end -}}

        ircconnect ircrelay // and tell it to connect!
    ]
    {{ end -}}
    """
    )


def make_additional_vars_section():
    text = textwrap.dedent(
        """
        {{ if (contains .Env "ADDITIONAL_VARS") -}}
        // additional variables
        {{ range $e := (split .Env.ADDITIONAL_VARS ";") -}}
        {{ $a := (split $e "=") -}}
        {{ index $a 0 }} {{ index $a 1 }}
        {{ end -}}
        {{ end -}}

        {{ if (contains .Env "SV_DUELMAXQUEUED") -}}
        sv_duelmaxqueued "{{ .Env.SV_DUELMAXQUEUED }}"
        {{ end -}}
        """
    )

    for i in ["duelmaxqueued", "teamneutralcolour"]:
        pass
        text += textwrap.dedent(
            """
            {{{{ if (contains .Env "SV_{upper}") -}}}}
            sv_{lower} "{{{{ .Env.SV_{upper} }}}}"
            {{{{ end -}}}}
            """.format(lower=i, upper=i.upper())
        )

    return text


def main():
    with open(sys.argv[1]) as f:
        lines = f.read().splitlines()

    for line in lines:
        print(parse_and_convert_line(line))

    print(make_irc_section())
    print(make_additional_vars_section())


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        pass
