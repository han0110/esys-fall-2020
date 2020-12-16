import { extendTheme } from '@chakra-ui/react'
import components from './components'
import styles from './styles'

const override = {
  config: {
    useSystemColorMode: false,
    initialColorMode: 'light',
  },
  components,
  styles,
}

export default extendTheme(override)
